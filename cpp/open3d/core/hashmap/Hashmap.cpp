// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

// High level non-templated hashmap interface for basic usages.

// If BUILD_CUDA_MODULE, link DefaultHashmap.cu that contains everything, and
// disable code inside DefaultHashmap.cpp
// Else, link DefaultHashmap.cpp and disregard DefaultHashmap.cu

#include "open3d/core/hashmap/Hashmap.h"

#include "open3d/core/Tensor.h"
#include "open3d/core/hashmap/DeviceHashmap.h"
#include "open3d/utility/Console.h"
#include "open3d/utility/Helper.h"

namespace open3d {
namespace core {

Hashmap::Hashmap(int64_t init_capacity,
                 Dtype dtype_key,
                 Dtype dtype_value,
                 const Device& device)
    : dtype_key_(dtype_key), dtype_value_(dtype_value) {
    device_hashmap_ = CreateDefaultDeviceHashmap(
            std::max(init_capacity / kDefaultElemsPerBucket, int64_t(1)),
            init_capacity, dtype_key.ByteSize(), dtype_value.ByteSize(),
            device);
}

void Hashmap::Rehash(int64_t buckets) {
    return device_hashmap_->Rehash(buckets);
}

void Hashmap::Insert(const void* input_keys,
                     const void* input_values,
                     addr_t* output_addrs,
                     bool* output_masks,
                     int64_t count) {
    return device_hashmap_->Insert(input_keys, input_values, output_addrs,
                                   output_masks, count);
}

void Hashmap::Insert(const Tensor& input_keys,
                     const Tensor& input_values,
                     Tensor& output_addrs,
                     Tensor& output_masks) {
    SizeVector input_key_elem_shape(input_keys.GetShape());
    input_key_elem_shape.erase(input_key_elem_shape.begin());
    AssertKeyDtype(input_keys.GetDtype(), input_key_elem_shape);

    SizeVector input_value_elem_shape(input_values.GetShape());
    input_value_elem_shape.erase(input_value_elem_shape.begin());
    AssertValueDtype(input_values.GetDtype(), input_value_elem_shape);

    SizeVector shape = input_keys.GetShape();
    if (shape.size() == 0 || shape[0] == 0) {
        utility::LogError("[Hashmap]: Invalid key tensor shape");
    }
    if (input_keys.GetDevice() != GetDevice()) {
        utility::LogError(
                "[Hashmap]: Incompatible key device, expected {}, but got {}",
                GetDevice().ToString(), input_keys.GetDevice().ToString());
    }

    SizeVector value_shape = input_values.GetShape();
    if (value_shape.size() == 0 || value_shape[0] != shape[0]) {
        utility::LogError("[Hashmap]: Invalid value tensor shape");
    }
    if (input_values.GetDevice() != GetDevice()) {
        utility::LogError(
                "[Hashmap]: Incompatible value device, expected {}, but got {}",
                GetDevice().ToString(), input_values.GetDevice().ToString());
    }

    int64_t count = shape[0];
    output_addrs = Tensor({count}, Dtype::Int32, GetDevice());
    output_masks = Tensor({count}, Dtype::Bool, GetDevice());

    Insert(input_keys.GetDataPtr(), input_values.GetDataPtr(),
           static_cast<addr_t*>(output_addrs.GetDataPtr()),
           static_cast<bool*>(output_masks.GetDataPtr()), count);
}

void Hashmap::Activate(const void* input_keys,
                       addr_t* output_addrs,
                       bool* output_masks,
                       int64_t count) {
    return device_hashmap_->Activate(input_keys, output_addrs, output_masks,
                                     count);
}

void Hashmap::Activate(const Tensor& input_keys,
                       Tensor& output_addrs,
                       Tensor& output_masks) {
    SizeVector input_key_elem_shape(input_keys.GetShape());
    input_key_elem_shape.erase(input_key_elem_shape.begin());
    AssertKeyDtype(input_keys.GetDtype(), input_key_elem_shape);

    SizeVector shape = input_keys.GetShape();
    if (shape.size() == 0 || shape[0] == 0) {
        utility::LogError("[Hashmap]: Invalid key tensor shape");
    }
    if (input_keys.GetDevice() != GetDevice()) {
        utility::LogError(
                "[Hashmap]: Incompatible device, expected {}, but got {}",
                GetDevice().ToString(), input_keys.GetDevice().ToString());
    }

    int64_t count = shape[0];

    output_addrs = Tensor({count}, Dtype::Int32, GetDevice());
    output_masks = Tensor({count}, Dtype::Bool, GetDevice());

    return Activate(input_keys.GetDataPtr(),
                    static_cast<addr_t*>(output_addrs.GetDataPtr()),
                    static_cast<bool*>(output_masks.GetDataPtr()), count);
}

void Hashmap::Find(const void* input_keys,
                   addr_t* output_addrs,
                   bool* output_masks,
                   int64_t count) {
    return device_hashmap_->Find(input_keys, output_addrs, output_masks, count);
}

void Hashmap::Find(const Tensor& input_keys,
                   Tensor& output_addrs,
                   Tensor& output_masks) {
    SizeVector input_key_elem_shape(input_keys.GetShape());
    input_key_elem_shape.erase(input_key_elem_shape.begin());
    AssertKeyDtype(input_keys.GetDtype(), input_key_elem_shape);

    SizeVector shape = input_keys.GetShape();
    if (shape.size() == 0 || shape[0] == 0) {
        utility::LogError("[Hashmap]: Invalid key tensor shape");
    }
    if (input_keys.GetDevice() != GetDevice()) {
        utility::LogError(
                "[Hashmap]: Incompatible device, expected {}, but got {}",
                GetDevice().ToString(), input_keys.GetDevice().ToString());
    }

    int64_t count = shape[0];

    output_masks = Tensor({count}, Dtype::Bool, GetDevice());
    output_addrs = Tensor({count}, Dtype::Int32, GetDevice());

    return Find(input_keys.GetDataPtr(),
                static_cast<addr_t*>(output_addrs.GetDataPtr()),
                static_cast<bool*>(output_masks.GetDataPtr()), count);
}

void Hashmap::Erase(const void* input_keys, bool* output_masks, int64_t count) {
    return device_hashmap_->Erase(input_keys, output_masks, count);
}

void Hashmap::Erase(const Tensor& input_keys, Tensor& output_masks) {
    SizeVector input_key_elem_shape(input_keys.GetShape());
    input_key_elem_shape.erase(input_key_elem_shape.begin());
    AssertKeyDtype(input_keys.GetDtype(), input_key_elem_shape);

    SizeVector shape = input_keys.GetShape();
    if (shape.size() == 0 || shape[0] == 0) {
        utility::LogError("[Hashmap]: Invalid key tensor shape");
    }
    if (input_keys.GetDevice() != GetDevice()) {
        utility::LogError(
                "[Hashmap]: Incompatible device, expected {}, but got {}",
                GetDevice().ToString(), input_keys.GetDevice().ToString());
    }

    int64_t count = shape[0];
    output_masks = Tensor({count}, Dtype::Bool, GetDevice());

    return Erase(input_keys.GetDataPtr(),
                 static_cast<bool*>(output_masks.GetDataPtr()), count);
}

int64_t Hashmap::GetActiveIndices(addr_t* output_addrs) {
    return device_hashmap_->GetActiveIndices(output_addrs);
}

int64_t Hashmap::Size() const { return device_hashmap_->Size(); }

int64_t Hashmap::GetCapacity() const { return device_hashmap_->GetCapacity(); }
int64_t Hashmap::GetBucketCount() const {
    return device_hashmap_->GetBucketCount();
}
Device Hashmap::GetDevice() const { return device_hashmap_->GetDevice(); }
int64_t Hashmap::GetKeyBytesize() const {
    return device_hashmap_->GetKeyBytesize();
}
int64_t Hashmap::GetValueBytesize() const {
    return device_hashmap_->GetValueBytesize();
}

Tensor& Hashmap::GetKeyTensor() { return device_hashmap_->GetKeyTensor(); }
Tensor& Hashmap::GetValueTensor() { return device_hashmap_->GetValueTensor(); }

/// Return number of elems per bucket.
/// High performance not required, so directly returns a vector.
std::vector<int64_t> Hashmap::BucketSizes() const {
    return device_hashmap_->BucketSizes();
};

/// Return size / bucket_count.
float Hashmap::LoadFactor() const { return device_hashmap_->LoadFactor(); }

void Hashmap::AssertKeyDtype(const Dtype& dtype_key,
                             const SizeVector& elem_shape) const {
    int64_t elem_byte_size = dtype_key.ByteSize() * elem_shape.NumElements();
    if (elem_byte_size != dtype_key_.ByteSize()) {
        utility::LogError(
                "[Hashmap] Inconsistent entry-wise byte size, expected {}, but "
                "got {}",
                dtype_key_.ByteSize(), elem_byte_size);
    }
}

void Hashmap::AssertValueDtype(const Dtype& dtype_value,
                               const SizeVector& elem_shape) const {
    int64_t elem_byte_size = dtype_value.ByteSize() * elem_shape.NumElements();
    if (elem_byte_size != dtype_value_.ByteSize()) {
        utility::LogError(
                "[Hashmap] Inconsistent entry-wise byte size, expected {}, but "
                "got {}",
                dtype_value_.ByteSize(), elem_byte_size);
    }
}

}  // namespace core
}  // namespace open3d
