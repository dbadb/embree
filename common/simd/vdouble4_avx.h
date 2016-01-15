// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

namespace embree
{ 
  /* 4-wide AVX 64bit double type */
  template<>
    struct vdouble<4>
  {
    typedef vboold4 Bool;

    enum  { size = 4 }; // number of SIMD elements
    union {              // data
      __m256d v; 
      double i[4]; 
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////
       
    __forceinline vdouble() {}
    __forceinline vdouble(const vdouble4& t) { v = t.v; }
    __forceinline vdouble4& operator=(const vdouble4& f) { v = f.v; return *this; }

    __forceinline vdouble(const __m256d& t) { v = t; }
    __forceinline operator __m256d () const { return v; }

    __forceinline vdouble(const double i) {
      v = _mm256_set1_pd(i);
    }
    
    __forceinline vdouble(const double a, const double b, const double c, const double d) {
      v = _mm256_set_pd(d,c,b,a);      
    }
   
    
    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////
    
    __forceinline vdouble( ZeroTy   ) : v(_mm256_setzero_pd()) {}
    __forceinline vdouble( OneTy    ) : v(_mm256_set1_pd(1)) {}
    __forceinline vdouble( StepTy   ) : v(_mm256_set_pd(3.0,2.0,1.0,0.0)) {}
    __forceinline vdouble( ReverseStepTy )   : v(_mm256_setr_pd(3.0,2.0,1.0,0.0)) {}

    __forceinline static vdouble4 zero() { return _mm256_setzero_pd(); }
    __forceinline static vdouble4 one () { return _mm256_set1_pd(1.0); }
    __forceinline static vdouble4 neg_one () { return _mm256_set1_pd(-1.0); }

    ////////////////////////////////////////////////////////////////////////////////
    /// Loads and Stores
    ////////////////////////////////////////////////////////////////////////////////

    static __forceinline void store_nt(double *__restrict__ ptr, const vdouble4& a) {
      _mm256_stream_pd(ptr,a);
    }

    static __forceinline vdouble4 loadu(const double* addr)
    {
      return _mm256_loadu_pd(addr);
    }

    static __forceinline vdouble4 load(const vdouble4* addr) {
      return _mm256_load_pd((double*)addr);
    }

    static __forceinline vdouble4 load(const double* addr) {
      return _mm256_load_pd(addr);
    }

    static __forceinline void store(double* ptr, const vdouble4& v) {
      _mm256_store_pd(ptr,v);
    }

    static __forceinline void storeu(double* ptr, const vdouble4& v ) {
      _mm256_storeu_pd(ptr,v);
    }


    ////////////////////////////////////////////////////////////////////////////////
    /// Array Access
    ////////////////////////////////////////////////////////////////////////////////
    
    __forceinline       double& operator[](const size_t index)       { assert(index < 8); return i[index]; }
    __forceinline const double& operator[](const size_t index) const { assert(index < 8); return i[index]; }

  };
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline const vdouble4 operator +( const vdouble4& a ) { return a; }
  __forceinline const vdouble4 operator -( const vdouble4& a ) { return _mm256_sub_pd(_mm256_setzero_pd(), a); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline const vdouble4 operator +( const vdouble4& a, const vdouble4& b )     { return _mm256_add_pd(a, b); }
  __forceinline const vdouble4 operator +( const vdouble4& a, const double b ) { return a + vdouble4(b); }
  __forceinline const vdouble4 operator +( const double a, const vdouble4& b ) { return vdouble4(a) + b; }

  __forceinline const vdouble4 operator -( const vdouble4& a, const vdouble4& b     ) { return _mm256_sub_pd(a, b); }
  __forceinline const vdouble4 operator -( const vdouble4& a, const double b ) { return a - vdouble4(b); }
  __forceinline const vdouble4 operator -( const double a, const vdouble4& b ) { return vdouble4(a) - b; }

  __forceinline const vdouble4 operator *( const vdouble4& a, const vdouble4& b )     { return _mm256_mul_pd(a, b); }
  __forceinline const vdouble4 operator *( const vdouble4& a, const double b ) { return a * vdouble4(b); }
  __forceinline const vdouble4 operator *( const double a, const vdouble4& b ) { return vdouble4(a) * b; }

  __forceinline const vdouble4 operator &( const vdouble4& a, const vdouble4& b )     { return _mm256_and_pd(a, b); }
  __forceinline const vdouble4 operator &( const vdouble4& a, const double b ) { return a & vdouble4(b); }
  __forceinline const vdouble4 operator &( const double a, const vdouble4& b ) { return vdouble4(a) & b; }

  __forceinline const vdouble4 operator |( const vdouble4& a, const vdouble4& b )     { return _mm256_or_pd(a, b); }
  __forceinline const vdouble4 operator |( const vdouble4& a, const double b ) { return a | vdouble4(b); }
  __forceinline const vdouble4 operator |( const double a, const vdouble4& b ) { return vdouble4(a) | b; }

  __forceinline const vdouble4 operator ^( const vdouble4& a, const vdouble4& b )     { return _mm256_xor_pd(a, b); }
  __forceinline const vdouble4 operator ^( const vdouble4& a, const double b ) { return a ^ vdouble4(b); }
  __forceinline const vdouble4 operator ^( const double a, const vdouble4& b ) { return vdouble4(a) ^ b; }
  
  __forceinline const vdouble4 min( const vdouble4& a, const vdouble4& b )     { return _mm256_min_pd(a, b); }
  __forceinline const vdouble4 min( const vdouble4& a, const double b ) { return min(a,vdouble4(b)); }
  __forceinline const vdouble4 min( const double a, const vdouble4& b ) { return min(vdouble4(a),b); }

  __forceinline const vdouble4 max( const vdouble4& a, const vdouble4& b )     { return _mm256_max_pd(a, b); }
  __forceinline const vdouble4 max( const vdouble4& a, const double b ) { return max(a,vdouble4(b)); }
  __forceinline const vdouble4 max( const double a, const vdouble4& b ) { return max(vdouble4(a),b); }
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Assignment Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vdouble4& operator +=( vdouble4& a, const vdouble4& b ) { return a = a + b; }
  __forceinline vdouble4& operator +=( vdouble4& a, const double     b ) { return a = a + b; }
  
  __forceinline vdouble4& operator -=( vdouble4& a, const vdouble4& b ) { return a = a - b; }
  __forceinline vdouble4& operator -=( vdouble4& a, const double     b ) { return a = a - b; }

  __forceinline vdouble4& operator *=( vdouble4& a, const vdouble4& b ) { return a = a * b; }
  __forceinline vdouble4& operator *=( vdouble4& a, const double     b ) { return a = a * b; }
  
  __forceinline vdouble4& operator &=( vdouble4& a, const vdouble4& b ) { return a = a & b; }
  __forceinline vdouble4& operator &=( vdouble4& a, const double     b ) { return a = a & b; }
  
  __forceinline vdouble4& operator |=( vdouble4& a, const vdouble4& b ) { return a = a | b; }
  __forceinline vdouble4& operator |=( vdouble4& a, const double     b ) { return a = a | b; }
  

  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline const vboold4 operator ==( const vdouble4& a, const vdouble4& b ) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_EQ); }
  __forceinline const vboold4 operator ==( const vdouble4& a, const double b ) { return a == vdouble4(b); }
  __forceinline const vboold4 operator ==( const double a, const vdouble4& b ) { return vdouble4(a) == b; }
  
  __forceinline const vboold4 operator !=( const vdouble4& a, const vdouble4& b ) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_NE); }
  __forceinline const vboold4 operator !=( const vdouble4& a, const double b ) { return a != vdouble4(b); }
  __forceinline const vboold4 operator !=( const double a, const vdouble4& b ) { return vdouble4(a) != b; }
  
  __forceinline const vboold4 operator < ( const vdouble4& a, const vdouble4& b ) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_LT); }
  __forceinline const vboold4 operator < ( const vdouble4& a, const double b ) { return a <  vdouble4(b); }
  __forceinline const vboold4 operator < ( const double a, const vdouble4& b ) { return vdouble4(a) <  b; }
  
  __forceinline const vboold4 operator >=( const vdouble4& a, const vdouble4& b ) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_GE); }
  __forceinline const vboold4 operator >=( const vdouble4& a, const double b ) { return a >= vdouble4(b); }
  __forceinline const vboold4 operator >=( const double a, const vdouble4& b ) { return vdouble4(a) >= b; }

  __forceinline const vboold4 operator > ( const vdouble4& a, const vdouble4& b ) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_GT); }
  __forceinline const vboold4 operator > ( const vdouble4& a, const double b ) { return a >  vdouble4(b); }
  __forceinline const vboold4 operator > ( const double a, const vdouble4& b ) { return vdouble4(a) >  b; }

  __forceinline const vboold4 operator <=( const vdouble4& a, const vdouble4& b ) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_LE); }
  __forceinline const vboold4 operator <=( const vdouble4& a, const double b ) { return a <= vdouble4(b); }
  __forceinline const vboold4 operator <=( const double a, const vdouble4& b ) { return vdouble4(a) <= b; }

  __forceinline vboold4 eq(                     const vdouble4& a, const vdouble4& b) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_EQ); }
  
  __forceinline vboold4 ne(                     const vdouble4& a, const vdouble4& b) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_NE); }

  __forceinline vboold4 lt(                     const vdouble4& a, const vdouble4& b) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_LT); }
 
  __forceinline vboold4 ge(                     const vdouble4& a, const vdouble4& b) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_GE); }
  
  __forceinline vboold4 gt(                     const vdouble4& a, const vdouble4& b) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_GT); }
  
  __forceinline vboold4 le(                     const vdouble4& a, const vdouble4& b) { return _mm256_cmp_pd_mask(a,b,_MM_CMPINT_LE); }
    
 
  __forceinline const vdouble4 select( const vboold4& m, const vdouble4& t, const vdouble4& f ) {
    return _mm256_blendv_pd(f, t, m);
  }

  __forceinline void xchg(const vboold4& m, vdouble4& a, vdouble4& b) {
    const vdouble4 c = a; a = select(m,b,a); b = select(m,c,b);
  }


  __forceinline vboold4 test(const vdouble4& a, const vdouble4& b) {
    return _mm256_testz_si256(_mm256_castpd_si256(a),_mm256_castpd_si256(b));
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////

  template<int D, int C, int B, int A> __forceinline vdouble4 shuffle   (const vdouble4& v) { return _mm256_permute4x64_pd(v,_MM_SHUFFLE(D,C,B,A)); }
  template<int A>                      __forceinline vdouble4 shuffle   (const vdouble4& x) { return shuffle<A,A,A,A>(v); }

  template<int i>
    __forceinline vdouble4 align_shift_right(const vdouble4& a, const vdouble4& b)
  {
    return _mm256_alignr_pd(a,b,i); 
  };

  __forceinline double toScalar(const vdouble4& a)
  {
    return _mm256_cvtsd_f64(a);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Reductions
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline double reduce_add(vdouble4 a) { return _mm256_reduce_add_pd(a); }
  __forceinline double reduce_min(vdouble4 a) { return _mm256_reduce_min_pd(a); }
  __forceinline double reduce_max(vdouble4 a) { return _mm256_reduce_max_pd(a); }
  
  __forceinline vdouble4 vreduce_min2(vdouble4 x) {                      return min(x,shuffle<2,3,0,1>(x)); }
  __forceinline vdouble4 vreduce_min (vdouble4 x) { x = vreduce_min2(x); return min(x,shuffle<1,0,3,2>(x)); }

  __forceinline vdouble4 vreduce_max2(vdouble4 x) {                      return max(x,shuffle<1,0,3,2>(x)); }
  __forceinline vdouble4 vreduce_max (vdouble4 x) { x = vreduce_max2(x); return max(x,shuffle<2,3,0,1>(x)); }

  __forceinline vdouble4 vreduce_and2(vdouble4 x) {                      return x & shuffle<1,0,3,2>(x); }
  __forceinline vdouble4 vreduce_and (vdouble4 x) { x = vreduce_and2(x); return x & shuffle<2,3,0,1>(x); }

  __forceinline vdouble4 vreduce_or2(vdouble4 x) {                     return x | shuffle<1,0,3,2>(x); }
  __forceinline vdouble4 vreduce_or (vdouble4 x) { x = vreduce_or2(x); return x | shuffle<2,3,0,1>(x); }

  __forceinline vdouble4 vreduce_add2(vdouble4 x) {                      return x + shuffle<1,0,3,2>(x); }
  __forceinline vdouble4 vreduce_add (vdouble4 x) { x = vreduce_add2(x); return x + shuffle<2,3,0,1>(x); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Memory load and store operations
  ////////////////////////////////////////////////////////////////////////////////


  
  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////
  
  __forceinline std::ostream& operator<<(std::ostream& cout, const vdouble4& v)
  {
    cout << "<" << v[0];
    for (size_t i=1; i<4; i++) cout << ", " << v[i];
    cout << ">";
    return cout;
  }
}