// Shim for <wrl/client.h>: Microsoft::WRL::ComPtr over IUnknown-style AddRef/Release.
#pragma once
#include "../win_compat.h"
namespace Microsoft { namespace WRL {
template <class T>
class ComPtr {
    T* p_ = nullptr;
public:
    ComPtr() {}
    ComPtr(T* p) : p_(p) { if (p_) p_->AddRef(); }
    ComPtr(const ComPtr& o) : p_(o.p_) { if (p_) p_->AddRef(); }
    ComPtr(ComPtr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    ~ComPtr() { Reset(); }
    ComPtr& operator=(const ComPtr& o) { if (this != &o) { Reset(); p_ = o.p_; if (p_) p_->AddRef(); } return *this; }
    ComPtr& operator=(ComPtr&& o) noexcept { if (this != &o) { Reset(); p_ = o.p_; o.p_ = nullptr; } return *this; }
    T* Get() const { return p_; }
    T* operator->() const { return p_; }
    T** GetAddressOf() { return &p_; }
    T** ReleaseAndGetAddressOf() { Reset(); return &p_; }
    T** operator&() { Reset(); return &p_; }
    bool operator==(decltype(nullptr)) const { return p_ == nullptr; }
    bool operator!=(decltype(nullptr)) const { return p_ != nullptr; }
    explicit operator bool() const { return p_ != nullptr; }
    T* Detach() { T* t = p_; p_ = nullptr; return t; }
    void Attach(T* p) { Reset(); p_ = p; }
    unsigned long Reset() { if (p_) { p_->Release(); p_ = nullptr; } return 0; }
};
}}
