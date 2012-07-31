//===- llvm/unittest/ADT/TinyPtrVectorTest.cpp ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TinyPtrVector unit tests.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/type_traits.h"
#include <algorithm>
#include <list>
#include <vector>

using namespace llvm;

namespace {

// The world's worst RNG, but it is deterministic and makes it easy to get
// *some* shuffling of elements.
static ptrdiff_t test_shuffle_rng(ptrdiff_t i) {
  return (i + i * 33) % i;
}
static ptrdiff_t (*test_shuffle_rng_p)(ptrdiff_t) = &test_shuffle_rng;

template <typename VectorT>
class TinyPtrVectorTest : public testing::Test {
protected:
  typedef typename VectorT::value_type PtrT;
  typedef typename remove_pointer<PtrT>::type ValueT;

  VectorT V;
  VectorT V2;

  ValueT TestValues[1024];
  std::vector<PtrT> TestPtrs;

  TinyPtrVectorTest() {
    for (size_t i = 0, e = array_lengthof(TestValues); i != e; ++i)
      TestPtrs.push_back(&TestValues[i]);

    std::random_shuffle(TestPtrs.begin(), TestPtrs.end(), test_shuffle_rng_p);
  }

  ArrayRef<PtrT> testArray(size_t N) {
    return makeArrayRef(&TestPtrs[0], N);
  }

  void appendValues(VectorT &V, ArrayRef<PtrT> Values) {
    for (size_t i = 0, e = Values.size(); i != e; ++i)
      V.push_back(Values[i]);
  }

  void expectValues(const VectorT &V, ArrayRef<PtrT> Values) {
    EXPECT_EQ(Values.empty(), V.empty());
    EXPECT_EQ(Values.size(), V.size());
    for (size_t i = 0, e = Values.size(); i != e; ++i) {
      EXPECT_EQ(Values[i], V[i]);
      EXPECT_EQ(Values[i], *llvm::next(V.begin(), i));
    }
    EXPECT_EQ(V.end(), llvm::next(V.begin(), Values.size()));
  }
};

typedef ::testing::Types<TinyPtrVector<int*>,
                         TinyPtrVector<double*>
                         > TinyPtrVectorTestTypes;
TYPED_TEST_CASE(TinyPtrVectorTest, TinyPtrVectorTestTypes);

TYPED_TEST(TinyPtrVectorTest, EmptyTest) {
  this->expectValues(this->V, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, PushPopBack) {
  this->V.push_back(this->TestPtrs[0]);
  this->expectValues(this->V, this->testArray(1));
  this->V.push_back(this->TestPtrs[1]);
  this->expectValues(this->V, this->testArray(2));
  this->V.push_back(this->TestPtrs[2]);
  this->expectValues(this->V, this->testArray(3));
  this->V.push_back(this->TestPtrs[3]);
  this->expectValues(this->V, this->testArray(4));
  this->V.push_back(this->TestPtrs[4]);
  this->expectValues(this->V, this->testArray(5));

  // Pop and clobber a few values to keep things interesting.
  this->V.pop_back();
  this->expectValues(this->V, this->testArray(4));
  this->V.pop_back();
  this->expectValues(this->V, this->testArray(3));
  this->TestPtrs[3] = &this->TestValues[42];
  this->TestPtrs[4] = &this->TestValues[43];
  this->V.push_back(this->TestPtrs[3]);
  this->expectValues(this->V, this->testArray(4));
  this->V.push_back(this->TestPtrs[4]);
  this->expectValues(this->V, this->testArray(5));

  this->V.pop_back();
  this->expectValues(this->V, this->testArray(4));
  this->V.pop_back();
  this->expectValues(this->V, this->testArray(3));
  this->V.pop_back();
  this->expectValues(this->V, this->testArray(2));
  this->V.pop_back();
  this->expectValues(this->V, this->testArray(1));
  this->V.pop_back();
  this->expectValues(this->V, this->testArray(0));

  this->appendValues(this->V, this->testArray(42));
  this->expectValues(this->V, this->testArray(42));
}

TYPED_TEST(TinyPtrVectorTest, ClearTest) {
  this->expectValues(this->V, this->testArray(0));
  this->V.clear();
  this->expectValues(this->V, this->testArray(0));

  this->appendValues(this->V, this->testArray(1));
  this->expectValues(this->V, this->testArray(1));
  this->V.clear();
  this->expectValues(this->V, this->testArray(0));

  this->appendValues(this->V, this->testArray(42));
  this->expectValues(this->V, this->testArray(42));
  this->V.clear();
  this->expectValues(this->V, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, CopyAndMoveCtorTest) {
  this->appendValues(this->V, this->testArray(42));
  TypeParam Copy(this->V);
  this->expectValues(Copy, this->testArray(42));

  // This is a separate copy, and so it shouldn't destroy the original.
  Copy.clear();
  this->expectValues(Copy, this->testArray(0));
  this->expectValues(this->V, this->testArray(42));

  TypeParam Copy2(this->V2);
  this->appendValues(Copy2, this->testArray(42));
  this->expectValues(Copy2, this->testArray(42));
  this->expectValues(this->V2, this->testArray(0));

#if LLVM_USE_RVALUE_REFERENCES
  TypeParam Move(std::move(Copy2));
  this->expectValues(Move, this->testArray(42));
  this->expectValues(Copy2, this->testArray(0));
#endif
}

TYPED_TEST(TinyPtrVectorTest, EraseTest) {
  this->appendValues(this->V, this->testArray(1));
  this->expectValues(this->V, this->testArray(1));
  this->V.erase(this->V.begin());
  this->expectValues(this->V, this->testArray(0));

  this->appendValues(this->V, this->testArray(42));
  this->expectValues(this->V, this->testArray(42));
  this->V.erase(this->V.begin());
  this->TestPtrs.erase(this->TestPtrs.begin());
  this->expectValues(this->V, this->testArray(41));
  this->V.erase(llvm::next(this->V.begin(), 1));
  this->TestPtrs.erase(llvm::next(this->TestPtrs.begin(), 1));
  this->expectValues(this->V, this->testArray(40));
  this->V.erase(llvm::next(this->V.begin(), 2));
  this->TestPtrs.erase(llvm::next(this->TestPtrs.begin(), 2));
  this->expectValues(this->V, this->testArray(39));
  this->V.erase(llvm::next(this->V.begin(), 5));
  this->TestPtrs.erase(llvm::next(this->TestPtrs.begin(), 5));
  this->expectValues(this->V, this->testArray(38));
  this->V.erase(llvm::next(this->V.begin(), 13));
  this->TestPtrs.erase(llvm::next(this->TestPtrs.begin(), 13));
  this->expectValues(this->V, this->testArray(37));

  typename TypeParam::iterator I = this->V.begin();
  do {
    I = this->V.erase(I);
  } while (I != this->V.end());
  this->expectValues(this->V, this->testArray(0));
}

}