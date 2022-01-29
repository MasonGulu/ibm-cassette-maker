#include "LL.h"
#include <cstddef>
#include <iostream>

using namespace std;

template <typename T>
void LL<T>::appendToEnd(T newData) {
    next = new LL<T>;
    next->data = newData;
}
