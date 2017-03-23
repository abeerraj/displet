#ifndef REVLIB_ALIGNED_MEMORY_H
#define REVLIB_ALIGNED_MEMORY_H

#include "Exception.h"

namespace rev {
    
const int REVLIB_ALIGNMENT_SIZE = 16;
    
template <typename T> class AlignedMemory {
public:
    AlignedMemory() : allocatedPointer_(0), alignedPointer_(0) {}
    AlignedMemory(const int size);
    ~AlignedMemory();
    
    void resize(const int size);
    
    T* pointer() { return alignedPointer_; }
    T* pointer() const { return alignedPointer_; }
    T& operator[](const int index) { return alignedPointer_[index]; }
    const T& operator[](const int index) const { return alignedPointer_[index]; }

private:
    void allocateMemory(const int size);
    void deleteMemory();
    
    T* allocatedPointer_;
    T* alignedPointer_;
};
    
template <typename T> AlignedMemory<T>::AlignedMemory(const int size) {
    allocateMemory(size);
}
    
template <typename T> AlignedMemory<T>::~AlignedMemory() {
    deleteMemory();
}
    
template <typename T> void AlignedMemory<T>::resize(const int size) {
    deleteMemory();
    allocateMemory(size);
}
    
template <typename T> void AlignedMemory<T>::allocateMemory(const int size) {
    allocatedPointer_ = new T [size + REVLIB_ALIGNMENT_SIZE];
    if (allocatedPointer_ == 0) {
        throw Exception("AlignedMemory::AlignedMemory", "Out of memory");
    }
    alignedPointer_ = reinterpret_cast<T*>((reinterpret_cast<size_t>(allocatedPointer_) + REVLIB_ALIGNMENT_SIZE - 1) & -REVLIB_ALIGNMENT_SIZE);
}
    
template <typename T> void AlignedMemory<T>::deleteMemory() {
    if (allocatedPointer_ != 0) {
        delete [] allocatedPointer_;
    }
}
    
}

#endif
