#include "lbm_cuda.hpp"
#include "cuda_utils.hpp"
#include <cuda_runtime.h>
#include <cuda_fp16.h>

namespace {
constexpr unsigned int kVelocitySet = 19u;
constexpr float kInv32768 = 3.0517578e-5f;

__device__ __forceinline__ float clampf(const float v, const float lo, const float hi) {
	return fminf(fmaxf(v, lo), hi);
}
__device__ __forceinline__ float sq(const float v) { return v*v; }
__device__ __forceinline__ float cb(const float v) { return v*v*v; }
__device__ __forceinline__ float signf(const float v) { return v < 0.0f ? -1.0f : 1.0f; }

struct Params {
	unsigned int Nx, Ny, Nz;
	unsigned long long N;
	float fx, fy, fz;
	float w;
	float def_6_sigma;
};

__device__ __forceinline__ unsigned long long index_f(const unsigned long long n, const unsigned int i, const unsigned long long N) {
	return (unsigned long long)i*N + n;
}

__device__ __forceinline__ void coordinates(const unsigned long long n, const unsigned int Nx, const unsigned int Ny, unsigned int* x, unsigned int* y, unsigned int* z) {
	const unsigned long long t = n % (unsigned long long)(Nx*Ny);
	*x = (unsigned int)(t % (unsigned long long)Nx);
	*y = (unsigned int)(t / (unsigned long long)Nx);
	*z = (unsigned int)(n / (unsigned long long)(Nx*Ny));
}

__device__ __forceinline__ void calculate_indices(const unsigned long long n, const unsigned int Nx, const unsigned int Ny, const unsigned int Nz,
	unsigned long long* x0, unsigned long long* xp, unsigned long long* xm,
	unsigned long long* y0, unsigned long long* yp, unsigned long long* ym,
	unsigned long long* z0, unsigned long long* zp, unsigned long long* zm) {
	unsigned int x, y, z;
	coordinates(n, Nx, Ny, &x, &y, &z);
	*x0 = (unsigned long long)x;
	*xp = (unsigned long long)((x+1u)%Nx);
	*xm = (unsigned long long)((x+Nx-1u)%Nx);
	*y0 = (unsigned long long)(y * Nx);
	*yp = (unsigned long long)(((y+1u)%Ny)*Nx);
	*ym = (unsigned long long)(((y+Ny-1u)%Ny)*Nx);
	*z0 = (unsigned long long)(z * (unsigned long long)(Ny*Nx));
	*zp = (unsigned long long)(((z+1u)%Nz) * (unsigned long long)(Ny*Nx));
	*zm = (unsigned long long)(((z+Nz-1u)%Nz) * (unsigned long long)(Ny*Nx));
}

__device__ __forceinline__ void neighbors(const unsigned long long n, const unsigned int Nx, const unsigned int Ny, const unsigned int Nz, unsigned long long* j) {
	unsigned long long x0, xp, xm, y0, yp, ym, z0, zp, zm;
	calculate_indices(n, Nx, Ny, Nz, &x0, &xp, &xm, &y0, &yp, &ym, &z0, &zp, &zm);
	j[0] = n;
	j[ 1] = xp+y0+z0; j[ 2] = xm+y0+z0;
	j[ 3] = x0+yp+z0; j[ 4] = x0+ym+z0;
	j[ 5] = x0+y0+zp; j[ 6] = x0+y0+zm;
	j[ 7] = xp+yp+z0; j[ 8] = xm+ym+z0;
	j[ 9] = xp+y0+zp; j[10] = xm+y0+zm;
	j[11] = x0+yp+zp; j[12] = x0+ym+zm;
	j[13] = xp+ym+z0; j[14] = xm+yp+z0;
	j[15] = xp+y0+zm; j[16] = xm+y0+zp;
	j[17] = x0+yp+zm; j[18] = x0+ym+zp;
}

__device__ __forceinline__ float load_fpxx(const __half* fi, const unsigned long long idx) {
	return __half2float(fi[idx]) * kInv32768;
}
__device__ __forceinline__ void store_fpxx(__half* fi, const unsigned long long idx, const float v) {
	fi[idx] = __float2half_rn(v * 32768.0f);
}

__device__ __forceinline__ void load_f(const unsigned long long n, float* fhn, const __half* fi, const unsigned long long* j, const unsigned long long t, const unsigned long long N) {
	fhn[0] = load_fpxx(fi, index_f(n, 0u, N));
	for(unsigned int i=1u; i<kVelocitySet; i+=2u) {
		fhn[i   ] = load_fpxx(fi, index_f(n   , t%2ull ? i    : i+1u, N));
		fhn[i+1u] = load_fpxx(fi, index_f(j[i], t%2ull ? i+1u : i   , N));
	}
}
__device__ __forceinline__ void store_f(const unsigned long long n, const float* fhn, __half* fi, const unsigned long long* j, const unsigned long long t, const unsigned long long N) {
	store_fpxx(fi, index_f(n, 0u, N), fhn[0]);
	for(unsigned int i=1u; i<kVelocitySet; i+=2u) {
		store_fpxx(fi, index_f(j[i], t%2ull ? i+1u : i   , N), fhn[i   ]);
		store_fpxx(fi, index_f(n   , t%2ull ? i    : i+1u, N), fhn[i+1u]);
	}
}
__device__ __forceinline__ void load_f_outgoing(const unsigned long long n, float* fon, const __half* fi, const unsigned long long* j, const unsigned long long t, const unsigned long long N) {
	for(unsigned int i=1u; i<kVelocitySet; i+=2u) {
		fon[i   ] = load_fpxx(fi, index_f(j[i], t%2ull ? i    : i+1u, N));
		fon[i+1u] = load_fpxx(fi, index_f(n   , t%2ull ? i+1u : i   , N));
	}
}
__device__ __forceinline__ void store_f_reconstructed(const unsigned long long n, const float* fhn, __half* fi, const unsigned long long* j, const unsigned long long t, const unsigned long long N, const unsigned char* flagsj_su) {
	for(unsigned int i=1u; i<kVelocitySet; i+=2u) {
		if(flagsj_su[i+1u]==0x20) store_fpxx(fi, index_f(n   , t%2ull ? i    : i+1u, N), fhn[i   ]);
		if(flagsj_su[i   ]==0x20) store_fpxx(fi, index_f(j[i], t%2ull ? i+1u : i   , N), fhn[i+1u]);
	}
}

__device__ __forceinline__ void calculate_f_eq(const float rho, float ux, float uy, float uz, float* feq, const float w0, const float ws, const float we) {
	const float rhom1 = rho - 1.0f;
	const float c3 = -3.0f*(sq(ux)+sq(uy)+sq(uz));
	uz *= 3.0f;
	ux *= 3.0f;
	uy *= 3.0f;
	feq[0] = w0*fmaf(rho, 0.5f*c3, rhom1);
	const float u0=ux+uy, u1=ux+uz, u2=uy+uz, u3=ux-uy, u4=ux-uz, u5=uy-uz;
	const float rhos=ws*rho, rhoe=we*rho, rhom1s=ws*rhom1, rhom1e=we*rhom1;
	feq[ 1] = fmaf(rhos, fmaf(0.5f, fmaf(ux, ux, c3), ux), rhom1s); feq[ 2] = fmaf(rhos, fmaf(0.5f, fmaf(ux, ux, c3), -ux), rhom1s);
	feq[ 3] = fmaf(rhos, fmaf(0.5f, fmaf(uy, uy, c3), uy), rhom1s); feq[ 4] = fmaf(rhos, fmaf(0.5f, fmaf(uy, uy, c3), -uy), rhom1s);
	feq[ 5] = fmaf(rhos, fmaf(0.5f, fmaf(uz, uz, c3), uz), rhom1s); feq[ 6] = fmaf(rhos, fmaf(0.5f, fmaf(uz, uz, c3), -uz), rhom1s);
	feq[ 7] = fmaf(rhoe, fmaf(0.5f, fmaf(u0, u0, c3), u0), rhom1e); feq[ 8] = fmaf(rhoe, fmaf(0.5f, fmaf(u0, u0, c3), -u0), rhom1e);
	feq[ 9] = fmaf(rhoe, fmaf(0.5f, fmaf(u1, u1, c3), u1), rhom1e); feq[10] = fmaf(rhoe, fmaf(0.5f, fmaf(u1, u1, c3), -u1), rhom1e);
	feq[11] = fmaf(rhoe, fmaf(0.5f, fmaf(u2, u2, c3), u2), rhom1e); feq[12] = fmaf(rhoe, fmaf(0.5f, fmaf(u2, u2, c3), -u2), rhom1e);
	feq[13] = fmaf(rhoe, fmaf(0.5f, fmaf(u3, u3, c3), u3), rhom1e); feq[14] = fmaf(rhoe, fmaf(0.5f, fmaf(u3, u3, c3), -u3), rhom1e);
	feq[15] = fmaf(rhoe, fmaf(0.5f, fmaf(u4, u4, c3), u4), rhom1e); feq[16] = fmaf(rhoe, fmaf(0.5f, fmaf(u4, u4, c3), -u4), rhom1e);
	feq[17] = fmaf(rhoe, fmaf(0.5f, fmaf(u5, u5, c3), u5), rhom1e); feq[18] = fmaf(rhoe, fmaf(0.5f, fmaf(u5, u5, c3), -u5), rhom1e);
}

__device__ __forceinline__ void calculate_rho_u(const float* f, float* rhon, float* uxn, float* uyn, float* uzn) {
	float rho=f[0];
	for(unsigned int i=1u; i<kVelocitySet; i++) rho += f[i];
	rho += 1.0f;
	const float ux = f[ 1]-f[ 2]+f[ 7]-f[ 8]+f[ 9]-f[10]+f[13]-f[14]+f[15]-f[16];
	const float uy = f[ 3]-f[ 4]+f[ 7]-f[ 8]+f[11]-f[12]+f[14]-f[13]+f[17]-f[18];
	const float uz = f[ 5]-f[ 6]+f[ 9]-f[10]+f[11]-f[12]+f[16]-f[15]+f[18]-f[17];
	*rhon = rho;
	*uxn = ux/rho;
	*uyn = uy/rho;
	*uzn = uz/rho;
}

__device__ __forceinline__ void calculate_forcing_terms(const float ux, const float uy, const float uz, const float fx, const float fy, const float fz, float* Fin, const float w0, const float ws, const float we) {
	const float uF = -0.33333334f*fmaf(ux, fx, fmaf(uy, fy, uz*fz));
	Fin[0] = 9.0f*w0*uF;
	for(unsigned int i=1u; i<kVelocitySet; i++) {
		const float cx = (i==1||i==7||i==9||i==13||i==15) ? 1.0f : (i==2||i==8||i==10||i==14||i==16) ? -1.0f : 0.0f;
		const float cy = (i==3||i==7||i==11||i==14||i==17) ? 1.0f : (i==4||i==8||i==12||i==13||i==18) ? -1.0f : 0.0f;
		const float cz = (i==5||i==9||i==11||i==16||i==18) ? 1.0f : (i==6||i==10||i==12||i==15||i==17) ? -1.0f : 0.0f;
		const float wi = (i<=6u) ? ws : we;
		Fin[i] = 9.0f*wi*fmaf(cx*fx+cy*fy+cz*fz, cx*ux+cy*uy+cz*uz+0.33333334f, uF);
	}
}

__device__ __forceinline__ void average_neighbors_non_gas(const unsigned long long n, const float* rho, const float* u, const unsigned char* flags,
	const unsigned int Nx, const unsigned int Ny, const unsigned int Nz, float* rhon, float* uxn, float* uyn, float* uzn) {
	unsigned long long j[kVelocitySet];
	neighbors(n, Nx, Ny, Nz, j);
	float rhot=0.0f, uxt=0.0f, uyt=0.0f, uzt=0.0f, counter=0.0f;
	for(unsigned int i=1u; i<kVelocitySet; i++) {
		const unsigned char flagsji_sus = flags[j[i]] & (0x38|0x01);
		if(flagsji_sus==0x08 || flagsji_sus==0x10 || flagsji_sus==0x18) {
			counter += 1.0f;
			rhot += rho[j[i]];
			uxt  += u[j[i]];
			uyt  += u[(unsigned long long)Nx*Ny*Nz + j[i]];
			uzt  += u[2ull*(unsigned long long)Nx*Ny*Nz + j[i]];
		}
	}
	*rhon = counter>0.0f ? rhot/counter : 1.0f;
	*uxn  = counter>0.0f ? uxt /counter : 0.0f;
	*uyn  = counter>0.0f ? uyt /counter : 0.0f;
	*uzn  = counter>0.0f ? uzt /counter : 0.0f;
}

__device__ __forceinline__ void average_neighbors_fluid(const unsigned long long n, const float* rho, const float* u, const unsigned char* flags,
	const unsigned int Nx, const unsigned int Ny, const unsigned int Nz, float* rhon, float* uxn, float* uyn, float* uzn) {
	unsigned long long j[kVelocitySet];
	neighbors(n, Nx, Ny, Nz, j);
	float rhot=0.0f, uxt=0.0f, uyt=0.0f, uzt=0.0f, counter=0.0f;
	for(unsigned int i=1u; i<kVelocitySet; i++) {
		const unsigned char flagsji_su = flags[j[i]] & 0x38;
		if(flagsji_su==0x08) {
			counter += 1.0f;
			rhot += rho[j[i]];
			uxt  += u[j[i]];
			uyt  += u[(unsigned long long)Nx*Ny*Nz + j[i]];
			uzt  += u[2ull*(unsigned long long)Nx*Ny*Nz + j[i]];
		}
	}
	*rhon = counter>0.0f ? rhot/counter : 1.0f;
	*uxn  = counter>0.0f ? uxt /counter : 0.0f;
	*uyn  = counter>0.0f ? uyt /counter : 0.0f;
	*uzn  = counter>0.0f ? uzt /counter : 0.0f;
}

__device__ __forceinline__ float calculate_phi(const float rhon, const float massn, const unsigned char flagsn) {
	return (flagsn&0x08) ? 1.0f : (flagsn&0x10) ? (rhon>0.0f ? clampf(massn/rhon, 0.0f, 1.0f) : 0.5f) : 0.0f;
}

__device__ __forceinline__ float3 make_float3v(const float x, const float y, const float z) {
	float3 v; v.x = x; v.y = y; v.z = z; return v;
}
__device__ __forceinline__ float dot3(const float3 a, const float3 b) {
	return a.x*b.x + a.y*b.y + a.z*b.z;
}
__device__ __forceinline__ float3 cross3(const float3 a, const float3 b) {
	return make_float3v(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}
__device__ __forceinline__ float3 normalize3(const float3 v) {
	const float len = rsqrtf(fmaxf(dot3(v, v), 1.0e-20f));
	return make_float3v(v.x*len, v.y*len, v.z*len);
}
__device__ __forceinline__ float fdimf_custom(const float a, const float b) {
	return fmaxf(a-b, 0.0f);
}

__device__ __forceinline__ void lu_solve(float* M, float* x, float* b, const int N, const int Nsol) {
	for(int i=0; i<Nsol; i++) {
		for(int j=i+1; j<Nsol; j++) {
			M[N*j+i] /= M[N*i+i];
			for(int k=i+1; k<Nsol; k++) M[N*j+k] -= M[N*j+i]*M[N*i+k];
		}
	}
	for(int i=0; i<Nsol; i++) {
		x[i] = b[i];
		for(int k=0; k<i; k++) x[i] -= M[N*i+k]*x[k];
	}
	for(int i=Nsol-1; i>=0; i--) {
		for(int k=i+1; k<Nsol; k++) x[i] -= M[N*i+k]*x[k];
		x[i] /= M[N*i+i];
	}
}

__device__ __forceinline__ float3 calculate_normal_py(const float* phij) {
	float3 n;
	n.x = 4.0f*(phij[ 2]-phij[ 1])+2.0f*(phij[ 8]-phij[ 7]+phij[10]-phij[ 9]+phij[14]-phij[13]+phij[16]-phij[15])+phij[20]-phij[19]+phij[22]-phij[21]+phij[24]-phij[23]+phij[25]-phij[26];
	n.y = 4.0f*(phij[ 4]-phij[ 3])+2.0f*(phij[ 8]-phij[ 7]+phij[12]-phij[11]+phij[13]-phij[14]+phij[18]-phij[17])+phij[20]-phij[19]+phij[22]-phij[21]+phij[23]-phij[24]+phij[26]-phij[25];
	n.z = 4.0f*(phij[ 6]-phij[ 5])+2.0f*(phij[10]-phij[ 9]+phij[12]-phij[11]+phij[15]-phij[16]+phij[17]-phij[18])+phij[20]-phij[19]+phij[21]-phij[22]+phij[24]-phij[23]+phij[26]-phij[25];
	return normalize3(n);
}

__device__ __forceinline__ float plic_cube_reduced(const float V, const float n1, const float n2, const float n3) {
	const float n12=n1+n2, n3V=n3*V;
	if(n12<=2.0f*n3V) return n3V+0.5f*n12;
	const float sqn1=sq(n1), n26=6.0f*n2, v1=sqn1/n26;
	if(v1<=n3V && n3V<v1+0.5f*(n2-n1)) return 0.5f*(n1+sqrtf(sqn1+8.0f*n2*(n3V-v1)));
	const float V6 = n1*n26*n3V;
	if(n3V<v1) return cbrtf(V6);
	const float v3 = n3<n12 ? (sq(n3)*(3.0f*n12-n3)+sqn1*(n1-3.0f*n3)+sq(n2)*(n2-3.0f*n3))/(n1*n26) : 0.5f*n12;
	const float sqn12=sqn1+sq(n2), V6cbn12=V6-cb(n1)-cb(n2);
	const bool case34 = n3V<v3;
	const float a = case34 ? V6cbn12 : 0.5f*(V6cbn12-cb(n3));
	const float b = case34 ?   sqn12 : 0.5f*(sqn12+sq(n3));
	const float c = case34 ?     n12 : 0.5f;
	const float t = sqrtf(sq(c)-b);
	return c-2.0f*t*sinf(0.33333334f*asinf((cb(c)-0.5f*a-1.5f*b*c)/cb(t)));
}

__device__ __forceinline__ float plic_cube(const float V0, const float3 n) {
	const float ax=fabsf(n.x), ay=fabsf(n.y), az=fabsf(n.z), V=0.5f-fabsf(V0-0.5f), l=ax+ay+az;
	const float n1 = fminf(fminf(ax, ay), az)/l;
	const float n3 = fmaxf(fmaxf(ax, ay), az)/l;
	const float n2 = fdimf_custom(1.0f, n1+n3);
	const float d = plic_cube_reduced(V, n1, n2, n3);
	return l*copysignf(0.5f-d, V0-0.5f);
}

__device__ __forceinline__ float c_D3Q27(const unsigned int i) {
	const float c[3*27] = {
		0, 1,-1, 0, 0, 0, 0, 1,-1, 1,-1, 0, 0, 1,-1, 1,-1, 0, 0, 1,-1, 1,-1, 1,-1,-1, 1,
		0, 0, 0, 1,-1, 0, 0, 1,-1, 0, 0, 1,-1,-1, 1, 0, 0, 1,-1, 1,-1, 1,-1,-1, 1, 1,-1,
		0, 0, 0, 0, 0, 1,-1, 0, 0, 1,-1, 1,-1, 0, 0,-1, 1,-1, 1, 1,-1,-1, 1, 1,-1, 1,-1
	};
	return c[i];
}

__device__ __forceinline__ void get_remaining_neighbor_phij(const unsigned long long n, const float* phit, const float* phi,
	const unsigned int Nx, const unsigned int Ny, const unsigned int Nz, float* phij) {
	unsigned long long x0, xp, xm, y0, yp, ym, z0, zp, zm;
	calculate_indices(n, Nx, Ny, Nz, &x0, &xp, &xm, &y0, &yp, &ym, &z0, &zp, &zm);
	unsigned long long j[8];
	j[0] = xp+yp+zp; j[1] = xm+ym+zm;
	j[2] = xp+yp+zm; j[3] = xm+ym+zp;
	j[4] = xp+ym+zp; j[5] = xm+yp+zm;
	j[6] = xm+yp+zp; j[7] = xp+ym+zm;
	for(unsigned int i=0u; i<19u; i++) phij[i] = phit[i];
	for(unsigned int i=19u; i<27u; i++) phij[i] = phi[j[i-19u]];
}

__device__ __forceinline__ float calculate_curvature(const unsigned long long n, const float* phit, const float* phi,
	const unsigned int Nx, const unsigned int Ny, const unsigned int Nz) {
	float phij[27];
	get_remaining_neighbor_phij(n, phit, phi, Nx, Ny, Nz, phij);
	const float3 bz = calculate_normal_py(phij);
	const float3 rn = make_float3v(0.56270900f, 0.32704452f, 0.75921047f);
	const float3 by = normalize3(cross3(bz, rn));
	const float3 bx = cross3(by, bz);
	unsigned int number = 0u;
	float3 p[24];
	const float center_offset = plic_cube(phij[0], bz);
	for(unsigned int i=1u; i<27u; i++) {
		if(phij[i]>0.0f && phij[i]<1.0f) {
			const float3 ei = make_float3v(c_D3Q27(i), c_D3Q27(27u+i), c_D3Q27(2u*27u+i));
			const float offset = plic_cube(phij[i], bz)-center_offset;
			p[number++] = make_float3v(dot3(ei, bx), dot3(ei, by), dot3(ei, bz)+offset);
		}
	}
	float M[25], x[5]={0.0f,0.0f,0.0f,0.0f,0.0f}, b[5]={0.0f,0.0f,0.0f,0.0f,0.0f};
	for(unsigned int i=0u; i<25u; i++) M[i] = 0.0f;
	for(unsigned int i=0u; i<number; i++) {
		const float x0=p[i].x, y0=p[i].y, z0=p[i].z, x2=x0*x0, y2=y0*y0, x3=x2*x0, y3=y2*y0;
		M[ 0]+=x2*x2; M[ 1]+=x2*y2; M[ 2]+=x3*y0; M[ 3]+=x3; M[ 4]+=x2*y0; b[0]+=x2*z0;
		M[ 6]+=y2*y2; M[ 7]+=x0*y3; M[ 8]+=x0*y2; M[ 9]+=y3; b[1]+=y2*z0;
		M[12]+=x2*y2; M[13]+=x2*y0; M[14]+=x0*y2; b[2]+=x0*y0*z0;
		M[18]+=x2; M[19]+=x0*y0; b[3]+=x0*z0;
		M[24]+=y2; b[4]+=y0*z0;
	}
	for(unsigned int i=1u; i<5u; i++) {
		for(unsigned int j=0u; j<i; j++) M[i*5u+j] = M[j*5u+i];
	}
	if(number>=5u) lu_solve(M, x, b, 5, 5);
	else lu_solve(M, x, b, 5, number>5u ? 5u : (int)number);
	const float A=x[0], B=x[1], C=x[2], H=x[3], I=x[4];
	const float K = (A*(I*I+1.0f)+B*(H*H+1.0f)-C*H*I)*cb(rsqrtf(H*H+I*I+1.0f));
	return clampf(K, -1.0f, 1.0f);
}

__global__ void initialize_kernel(__half* fi, const float* rho, float* u, unsigned char* flags, float* mass, float* massex, float* phi, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	unsigned char flagsn = flags[n];
	const unsigned char flagsn_bo = flagsn & 0x01;
	unsigned long long j[kVelocitySet];
	neighbors(n, params.Nx, params.Ny, params.Nz, j);
	unsigned char flagsj[kVelocitySet];
	for(unsigned int i=1u; i<kVelocitySet; i++) flagsj[i] = flags[j[i]];
	if(flagsn_bo==0x01) {
		u[n] = 0.0f;
		u[params.N + n] = 0.0f;
		u[2ull*params.N + n] = 0.0f;
	}
	float feq[kVelocitySet];
	const float w0 = 1.0f/3.0f;
	const float ws = 1.0f/18.0f;
	const float we = 1.0f/36.0f;
	calculate_f_eq(rho[n], u[n], u[params.N+n], u[2ull*params.N+n], feq, w0, ws, we);
	float phin = phi[n];
	if(!(flagsn&(0x01|0x08|0x10))) flagsn = (flagsn&~0x38)|0x20;
	if((flagsn&0x38)==0x20) {
		bool change = false;
		for(unsigned int i=1u; i<kVelocitySet; i++) change = change || ((flagsj[i]&0x38)==0x08);
		if(change) {
			flagsn = (flagsn&~0x38)|0x10;
			phin = 0.5f;
			float rhon, uxn, uyn, uzn;
			average_neighbors_fluid(n, rho, u, flags, params.Nx, params.Ny, params.Nz, &rhon, &uxn, &uyn, &uzn);
			calculate_f_eq(rhon, uxn, uyn, uzn, feq, w0, ws, we);
		}
	}
	if((flagsn&0x38)==0x20) {
		u[n] = 0.0f;
		u[params.N + n] = 0.0f;
		u[2ull*params.N + n] = 0.0f;
		phin = 0.0f;
	} else if((flagsn&0x38)==0x10 && (phin<0.0f || phin>1.0f)) {
		phin = 0.5f;
	} else if((flagsn&0x38)==0x08) {
		phin = 1.0f;
	}
	phi[n] = phin;
	mass[n] = phin*rho[n];
	massex[n] = 0.0f;
	flags[n] = flagsn;
	store_f(n, feq, fi, j, 1ull, params.N);
}

__global__ void stream_collide_kernel(__half* fi, float* rho, float* u, unsigned char* flags, const unsigned long long t, const float fx, const float fy, const float fz, const float* mass, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	const unsigned char flagsn = flags[n];
	const unsigned char flagsn_bo = flagsn & 0x01;
	const unsigned char flagsn_su = flagsn & 0x38;
	if(flagsn_bo==0x01 || flagsn_su==0x20) return;
	unsigned long long j[kVelocitySet];
	neighbors(n, params.Nx, params.Ny, params.Nz, j);
	float fhn[kVelocitySet];
	load_f(n, fhn, fi, j, t, params.N);
	float rhon, uxn, uyn, uzn;
	calculate_rho_u(fhn, &rhon, &uxn, &uyn, &uzn);
	if(flagsn_su==0x10) {
		bool no_f = true, no_g = true;
		for(unsigned int i=1u; i<kVelocitySet; i++) {
			const unsigned char flagsji_su = flags[j[i]] & 0x38;
			no_f = no_f && flagsji_su!=0x08;
			no_g = no_g && flagsji_su!=0x20;
		}
		const float massn = mass[n];
		if(massn>rhon || no_g) flags[n] = (flagsn&~0x38)|0x18;
		else if(massn<0.0f || no_f) flags[n] = (flagsn&~0x38)|0x30;
	}
	float Fin[kVelocitySet];
	const float rho2 = 0.5f/rhon;
	uxn = clampf(fmaf(fx, rho2, uxn), -0.57735027f, 0.57735027f);
	uyn = clampf(fmaf(fy, rho2, uyn), -0.57735027f, 0.57735027f);
	uzn = clampf(fmaf(fz, rho2, uzn), -0.57735027f, 0.57735027f);
	calculate_forcing_terms(uxn, uyn, uzn, fx, fy, fz, Fin, 1.0f/3.0f, 1.0f/18.0f, 1.0f/36.0f);
	rho[n] = rhon;
	u[n] = uxn;
	u[params.N+n] = uyn;
	u[2ull*params.N+n] = uzn;
	float feq[kVelocitySet];
	calculate_f_eq(rhon, uxn, uyn, uzn, feq, 1.0f/3.0f, 1.0f/18.0f, 1.0f/36.0f);
	const float c_tau = fmaf(params.w, -0.5f, 1.0f);
	for(unsigned int i=0u; i<kVelocitySet; i++) Fin[i] *= c_tau;
	for(unsigned int i=0u; i<kVelocitySet; i++) fhn[i] = fmaf(1.0f-params.w, fhn[i], fmaf(params.w, feq[i], Fin[i]));
	store_f(n, fhn, fi, j, t, params.N);
}

__global__ void update_fields_kernel(const __half* fi, float* rho, float* u, const unsigned char* flags, const unsigned long long t, const float fx, const float fy, const float fz, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	const unsigned char flagsn = flags[n];
	const unsigned char flagsn_bo = flagsn & 0x01;
	const unsigned char flagsn_su = flagsn & 0x38;
	if(flagsn_bo==0x01 || flagsn_su==0x20) return;
	unsigned long long j[kVelocitySet];
	neighbors(n, params.Nx, params.Ny, params.Nz, j);
	float fhn[kVelocitySet];
	load_f(n, fhn, fi, j, t, params.N);
	float rhon, uxn, uyn, uzn;
	calculate_rho_u(fhn, &rhon, &uxn, &uyn, &uzn);
	const float rho2 = 0.5f/rhon;
	uxn = clampf(fmaf(fx, rho2, uxn), -0.57735027f, 0.57735027f);
	uyn = clampf(fmaf(fy, rho2, uyn), -0.57735027f, 0.57735027f);
	uzn = clampf(fmaf(fz, rho2, uzn), -0.57735027f, 0.57735027f);
	rho[n] = rhon;
	u[n] = uxn;
	u[params.N+n] = uyn;
	u[2ull*params.N+n] = uzn;
}

__global__ void surface_0_kernel(__half* fi, const float* rho, const float* u, const unsigned char* flags, float* mass, const float* massex, const float* phi, const unsigned long long t, const float fx, const float fy, const float fz, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	const unsigned char flagsn = flags[n];
	const unsigned char flagsn_bo = flagsn & 0x01;
	const unsigned char flagsn_su = flagsn & 0x38;
	if(flagsn_bo==0x01 || flagsn_su==0x20) return;
	unsigned long long j[kVelocitySet];
	neighbors(n, params.Nx, params.Ny, params.Nz, j);
	float fhn[kVelocitySet];
	load_f(n, fhn, fi, j, t, params.N);
	float fon[kVelocitySet];
	fon[0] = fhn[0];
	load_f_outgoing(n, fon, fi, j, t, params.N);
	float massn = mass[n];
	for(unsigned int i=1u; i<kVelocitySet; i++) {
		massn += massex[j[i]];
	}
	if(flagsn_su==0x08) {
		for(unsigned int i=1u; i<kVelocitySet; i++) massn += fhn[i]-fon[i];
	} else if(flagsn_su==0x10) {
		float phij[kVelocitySet];
		for(unsigned int i=1u; i<kVelocitySet; i++) phij[i] = phi[j[i]];
		float rhon, uxn, uyn, uzn;
		calculate_rho_u(fon, &rhon, &uxn, &uyn, &uzn);
		uxn = clampf(uxn, -0.57735027f, 0.57735027f);
		uyn = clampf(uyn, -0.57735027f, 0.57735027f);
		uzn = clampf(uzn, -0.57735027f, 0.57735027f);
		phij[0] = calculate_phi(rhon, massn, flagsn);
		float rho_laplace = params.def_6_sigma==0.0f ? 0.0f : params.def_6_sigma*calculate_curvature(n, phij, phi, params.Nx, params.Ny, params.Nz);
		float feg[kVelocitySet];
		const float rho2tmp = 0.5f/rhon;
		const float uxntmp = clampf(fmaf(fx, rho2tmp, uxn), -0.57735027f, 0.57735027f);
		const float uyntmp = clampf(fmaf(fy, rho2tmp, uyn), -0.57735027f, 0.57735027f);
		const float uzntmp = clampf(fmaf(fz, rho2tmp, uzn), -0.57735027f, 0.57735027f);
		calculate_f_eq(1.0f-rho_laplace, uxntmp, uyntmp, uzntmp, feg, 1.0f/3.0f, 1.0f/18.0f, 1.0f/36.0f);
		unsigned char flagsj_su[kVelocitySet];
		for(unsigned int i=1u; i<kVelocitySet; i++) flagsj_su[i] = flags[j[i]] & 0x38;
		for(unsigned int i=1u; i<kVelocitySet; i+=2u) {
			massn += (flagsj_su[i] & (0x08|0x10)) ? (flagsj_su[i]==0x08 ? fhn[i+1]-fon[i] : 0.5f*(phij[i]+phij[0])*(fhn[i+1]-fon[i])) : 0.0f;
			massn += (flagsj_su[i+1u] & (0x08|0x10)) ? (flagsj_su[i+1u]==0x08 ? fhn[i]-fon[i+1u] : 0.5f*(phij[i+1u]+phij[0])*(fhn[i]-fon[i+1u])) : 0.0f;
		}
		for(unsigned int i=1u; i<kVelocitySet; i+=2u) {
			fhn[i] = feg[i+1u]-fon[i+1u]+feg[i];
			fhn[i+1u] = feg[i]-fon[i]+feg[i+1u];
		}
		store_f_reconstructed(n, fhn, fi, j, t, params.N, flagsj_su);
	}
	mass[n] = massn;
}

__global__ void surface_1_kernel(unsigned char* flags, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	const unsigned char flagsn_sus = flags[n] & (0x38|0x01);
	if(flagsn_sus==0x18) {
		unsigned long long j[kVelocitySet];
		neighbors(n, params.Nx, params.Ny, params.Nz, j);
		for(unsigned int i=1u; i<kVelocitySet; i++) {
			const unsigned char flagsji = flags[j[i]];
			const unsigned char flagsji_su = flagsji & (0x38|0x01);
			const unsigned char flagsji_r = flagsji & ~0x38;
			if(flagsji_su==0x30) flags[j[i]] = flagsji_r|0x10;
			else if(flagsji_su==0x20) flags[j[i]] = flagsji_r|0x38;
		}
	}
}

__global__ void surface_2_kernel(__half* fi, const float* rho, const float* u, unsigned char* flags, const unsigned long long t, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	const unsigned char flagsn_sus = flags[n] & (0x38|0x01);
	if(flagsn_sus==0x38) {
		float rhon, uxn, uyn, uzn;
		average_neighbors_non_gas(n, rho, u, flags, params.Nx, params.Ny, params.Nz, &rhon, &uxn, &uyn, &uzn);
		float feq[kVelocitySet];
		calculate_f_eq(rhon, uxn, uyn, uzn, feq, 1.0f/3.0f, 1.0f/18.0f, 1.0f/36.0f);
		unsigned long long j[kVelocitySet];
		neighbors(n, params.Nx, params.Ny, params.Nz, j);
		store_f(n, feq, fi, j, t, params.N);
	} else if(flagsn_sus==0x30) {
		unsigned long long j[kVelocitySet];
		neighbors(n, params.Nx, params.Ny, params.Nz, j);
		for(unsigned int i=1u; i<kVelocitySet; i++) {
			const unsigned char flagsji = flags[j[i]];
			const unsigned char flagsji_su = flagsji & (0x38|0x01);
			const unsigned char flagsji_r = flagsji & ~0x38;
			if(flagsji_su==0x08 || flagsji_su==0x18) {
				flags[j[i]] = flagsji_r|0x10;
			}
		}
	}
}

__global__ void surface_3_kernel(const float* rho, unsigned char* flags, float* mass, float* massex, float* phi, Params params) {
	const unsigned long long n = (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
	if(n >= params.N) return;
	const unsigned char flagsn_sus = flags[n] & (0x38|0x01);
	if(flagsn_sus & 0x01) return;
	const float rhon = rho[n];
	float massn = mass[n];
	float massexn = 0.0f;
	float phin = 0.0f;
	if(flagsn_sus==0x08) {
		massexn = massn - rhon;
		massn = rhon;
		phin = 1.0f;
	} else if(flagsn_sus==0x10) {
		massexn = massn>rhon ? massn-rhon : massn<0.0f ? massn : 0.0f;
		massn = clampf(massn, 0.0f, rhon);
		phin = calculate_phi(rhon, massn, 0x10);
	} else if(flagsn_sus==0x20) {
		massexn = massn;
		massn = 0.0f;
		phin = 0.0f;
	} else if(flagsn_sus==0x18) {
		flags[n] = (flags[n]&~0x38)|0x08;
		massexn = massn - rhon;
		massn = rhon;
		phin = 1.0f;
	} else if(flagsn_sus==0x30) {
		flags[n] = (flags[n]&~0x38)|0x20;
		massexn = massn;
		massn = 0.0f;
		phin = 0.0f;
	} else if(flagsn_sus==0x38) {
		flags[n] = (flags[n]&~0x38)|0x10;
		massexn = massn>rhon ? massn-rhon : massn<0.0f ? massn : 0.0f;
		massn = clampf(massn, 0.0f, rhon);
		phin = calculate_phi(rhon, massn, 0x10);
	}
	unsigned long long j[kVelocitySet];
	neighbors(n, params.Nx, params.Ny, params.Nz, j);
	unsigned int counter = 0u;
	for(unsigned int i=1u; i<kVelocitySet; i++) {
		const unsigned char flagsji_su = flags[j[i]] & (0x38|0x01);
		counter += (unsigned int)(flagsji_su==0x08 || flagsji_su==0x10 || flagsji_su==0x18 || flagsji_su==0x38);
	}
	massn += counter>0u ? 0.0f : massexn;
	massexn = counter>0u ? massexn/(float)counter : 0.0f;
	mass[n] = massn;
	massex[n] = massexn;
	phi[n] = phin;
}

} // namespace

CudaLBMBackend::~CudaLBMBackend() {
	if(fi) cudaFree(fi);
	if(rho) cudaFree(rho);
	if(u) cudaFree(u);
	if(flags) cudaFree(flags);
	if(mass) cudaFree(mass);
	if(massex) cudaFree(massex);
	if(phi) cudaFree(phi);
}

void CudaLBMBackend::initialize(const CudaLBMParams& p) {
	params = p;
	if(allocated) return;
	const size_t N = params.N;
	CUDA_CHECK(cudaMalloc(&fi, sizeof(__half)*N*kVelocitySet));
	CUDA_CHECK(cudaMalloc(&rho, sizeof(float)*N));
	CUDA_CHECK(cudaMalloc(&u, sizeof(float)*N*3ull));
	CUDA_CHECK(cudaMalloc(&flags, sizeof(unsigned char)*N));
	CUDA_CHECK(cudaMalloc(&mass, sizeof(float)*N));
	CUDA_CHECK(cudaMalloc(&massex, sizeof(float)*N));
	CUDA_CHECK(cudaMalloc(&phi, sizeof(float)*N));
	allocated = true;
}

void CudaLBMBackend::upload_host_fields(const float* rho_h, const float* u_h, const unsigned char* flags_h, const float* phi_h) {
	if(!allocated) return;
	CUDA_CHECK(cudaMemcpy(rho, rho_h, sizeof(float)*params.N, cudaMemcpyHostToDevice));
	CUDA_CHECK(cudaMemcpy(u, u_h, sizeof(float)*params.N*3ull, cudaMemcpyHostToDevice));
	CUDA_CHECK(cudaMemcpy(flags, flags_h, sizeof(unsigned char)*params.N, cudaMemcpyHostToDevice));
	CUDA_CHECK(cudaMemcpy(phi, phi_h, sizeof(float)*params.N, cudaMemcpyHostToDevice));
}

void CudaLBMBackend::kernel_initialize() {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	initialize_kernel<<<grid, block>>>((__half*)fi, rho, u, flags, mass, massex, phi, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::kernel_stream_collide(const unsigned long long t) {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	stream_collide_kernel<<<grid, block>>>((__half*)fi, rho, u, flags, t, params.fx, params.fy, params.fz, mass, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::kernel_surface_capture_outgoing(const unsigned long long t) {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	surface_0_kernel<<<grid, block>>>((__half*)fi, rho, u, flags, mass, massex, phi, t, params.fx, params.fy, params.fz, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::kernel_surface_mass_exchange() {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	surface_1_kernel<<<grid, block>>>(flags, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::kernel_surface_flag_transition(const unsigned long long t) {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	surface_2_kernel<<<grid, block>>>((__half*)fi, rho, u, flags, t, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::kernel_surface_phi_recompute() {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	surface_3_kernel<<<grid, block>>>(rho, flags, mass, massex, phi, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::kernel_update_fields(const unsigned long long t) {
	const dim3 block(256);
	const dim3 grid((unsigned int)((params.N + block.x - 1ull) / block.x));
	update_fields_kernel<<<grid, block>>>((__half*)fi, rho, u, flags, t, params.fx, params.fy, params.fz, Params{params.Nx, params.Ny, params.Nz, params.N, params.fx, params.fy, params.fz, params.w, params.def_6_sigma});
	CUDA_CHECK(cudaGetLastError());
}

void CudaLBMBackend::download_fields(float* rho_h, float* u_h, unsigned char* flags_h, float* phi_h) const {
	if(!allocated) return;
	CUDA_CHECK(cudaMemcpy(rho_h, rho, sizeof(float)*params.N, cudaMemcpyDeviceToHost));
	CUDA_CHECK(cudaMemcpy(u_h, u, sizeof(float)*params.N*3ull, cudaMemcpyDeviceToHost));
	CUDA_CHECK(cudaMemcpy(flags_h, flags, sizeof(unsigned char)*params.N, cudaMemcpyDeviceToHost));
	CUDA_CHECK(cudaMemcpy(phi_h, phi, sizeof(float)*params.N, cudaMemcpyDeviceToHost));
}

void CudaLBMBackend::synchronize() const {
	CUDA_CHECK(cudaDeviceSynchronize());
}
