#include "mdp_psim.h"
#include "math.h"

/// assembly macro for fast products using SSE SIMD
/// c0=r0*s0,c1=r1*s1,c2=r2*s2,c3=r3*s3
#define _sse_float_scalar_product(r,s,c) { \
asm("movups %1, %%xmm0 \n\t"               \
    "movups %2, %%xmm1 \n\t"	           \
    "mulps %%xmm0, %%xmm1 \n\t"		   \
    "movups %0, %%xmm2\n\t"		   \
    "addps %%xmm1, %%xmm2 \n\t"		   \
    "movups %%xmm2, %0 \n\t"		   \
    : "=m"(*c) : "m" (*r), "m" (*s)); }

/// regular function to initialize a vector data
void initialize(float *data, int size, int start=0, float phase=1.0) {
  for(int i=0; i<size;i++) {
    data[i]=sin(phase*(i+start));
  }
}

/// naive scalar product (as anybody whould write)
float product_simple(float *data1, float *data2, int size) {
  float sum=0;
  for(int i=0; i<size; i++)
    sum+=data1[i]*data2[i];
  return sum;
}

/// scalar product using P4 SSE SIMD instructions
float product_sse(float *data1, float *data2, int size) {
  float sum=0;
  float data3[4]={0,0,0,0};
  register float *p1=data1, *p2=data2;
  register int i;
  for(i=0; i<size-3; i+=4, p1+=4, p2+=4) {
    _sse_float_scalar_product(p1,p2,data3);
  }
  sum=data3[0]+data3[1]+data3[2]+data3[3];
  for(; i<size; i++)
    sum+=data1[i]*data2[i];  
  return sum;
}

struct tdata {
  pthread_t id;
  pthread_attr_t attr;
  float *data1;
  float *data2;
  int size;
  float sum;
};

void* mythread(void* p) {
  tdata& mydata=*((tdata*) p);
  // cout << mydata.id << " " << mydata.size << endl;
  mydata.sum=product_simple(mydata.data1,mydata.data2,mydata.size);
  pthread_exit(NULL);
  return 0;
}

/// scalar product using threads (MIMD on shared memory architecture)
float product_thread(float *data1, float *data2, int size, int nthreads,
		     void* (*thread)(void*)=mythread) {
  int size2=size;
  vector<tdata> v(nthreads);
  for(int k=0; k<nthreads; k++) {
    pthread_attr_init(&v[k].attr);
    v[k].data1=data1+k*((int) size/nthreads);
    v[k].data2=data2+k*((int) size/nthreads);
    if(k<nthreads-1) v[k].size=(int) size/nthreads;
    else v[k].size=size-k*((int) size/nthreads);
    pthread_create(&v[k].id,&v[k].attr,thread,(void*) &v[k]);
  }  

  float sum=0.0;
  for(int k=0; k<nthreads; k++) {
    pthread_join(v[k].id,0);
    sum=sum+v[k].sum;
  }  
  return sum;
}

void* mythread_sse(void* p) {
  tdata& mydata=*((tdata*) p);
  // cout << mydata.id << " " << mydata.size << endl;
  mydata.sum=product_sse(mydata.data1,mydata.data2,mydata.size);
  pthread_exit(NULL);
  return 0;
}

/// scalar product using threads and SSE 
/// (MIMD+SIMD on shared memory architecture compatible with P4)
float product_thread_sse(float *data1, float *data2, int size, int nthreads) {
  return product_thread(data1,data2,size,nthreads,mythread_sse);
}

/// scalar product using simulated parallel processes (pure MIMD)
float product_fork(float *data1, float *data2, int size, int nprocs) {
  float sum;
  mdp_psim node(nprocs,"",1);
  int size1=size/nprocs;
  int size2=size1;
  if(node.id()==nprocs-1) size2=size-(nprocs-1)*size1;
  sum=product_simple(data1+node.id()*size1,
		     data2+node.id()*size1,
		     size2);
  sum=node.add(sum);
  return sum;
}

/// scalar product using simulated parallel processes on P4 (MIMD+SIMD)
float product_fork_sse(float *data1, float *data2, int size, int nprocs) {
  float sum;
  mdp_psim node(nprocs,"",1);
  int size1=size/nprocs;
  int size2=size1;
  if(node.id()==nprocs-1) size2=size-(nprocs-1)*size1;
  sum=product_sse(data1+node.id()*size1,
		  data2+node.id()*size1,
		  size2);
  sum=node.add(sum);
  return sum;
}

/// scalar product using simulated parallel processes and threads (MIMD)
float product_fork_thread(float *data1, float *data2, int size, int nprocs, 
			  int nthreads) {
  float sum;
  mdp_psim node(nprocs,"",1);
  int size1=size/nprocs;
  int size2=size1;
  if(node.id()==nprocs-1) size2=size-(nprocs-1)*size1;
  sum=product_thread(data1+node.id()*size1,
		     data2+node.id()*size1,
		     size2,nthreads);
  sum=node.add(sum);
  return sum;
}

/// scalar product using simulated parallel processes, threads and SSE 
/// (MIMD+SIMD)
float product_fork_thread_sse(float *data1, float *data2, 
			      int size, int nprocs, int nthreads) {
  float sum;
  mdp_psim node(nprocs,"",1);
  int size1=size/nprocs;
  int size2=size1;
  if(node.id()==nprocs-1) size2=size-(nprocs-1)*size1;
  sum=product_thread_sse(data1+node.id()*size1,
			 data2+node.id()*size1,
			 size2,nthreads);
  sum=node.add(sum);
  return sum;
}

int main(int argc, char** argv) {
  if(argc < 2) {
    cout << "Usage ./a.exe x\n"
	 << "with x = 0 for simple\n"
	 << "     x = 1 for sse\n"
	 << "     x = 2 for 2 threads\n"
	 << "     x = 3 for 2 threads with sse\n"
	 << "     x = 4 for 2 sim.procs. \n"
	 << "     x = 5 for 2 sim.procs. with sse\n"
	 << "     x = 6 for 2 sim.procs. x 2 threads \n"
	 << "     x = 7 for 2 sim.procs. x 2 threads with sse\n";
    return 0;
  }
  const int n=5000000;
  cout << "allocating memory...\n";
  float *data1=new float[n];
  float *data2=new float[n];
  cout << "done.\ninitializing vectors...\n";
  initialize(data1,n,0,2.0);
  initialize(data2,n,0,3.0);
  cout << "done.\nstart scalar product (timing)...\n";
  float sum;
  float t0=clock();

  switch(argv[1][0]) {
  case '0': sum=product_simple(data1,data2,n); break;
  case '1': sum=product_sse(data1,data2,n); break;
  case '2': sum=product_thread(data1,data2,n,2); break;
  case '3': sum=product_thread_sse(data1,data2,n,2); break;
  case '4': sum=product_fork(data1,data2,n,2); break;
  case '5': sum=product_fork_sse(data1,data2,n,2); break;
  case '6': sum=product_fork_thread(data1,data2,n,2,2); break;
  case '7': sum=product_fork_thread_sse(data1,data2,n,2,2); break;
  }
  t0=clock()-t0;
  cout << "done.\nscalar product="<<sum<<"\n";
  cout << "computed in " << t0 << " CPU cycles\n";
  cout << "cleaning memory...\n";
  delete[] data1;
  delete[] data2;
  cout << "done!\n";
  return 0;
}
