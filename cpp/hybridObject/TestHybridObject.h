#pragma once

#include "HybridObject.h"

namespace RNWorklet
{
    class TestHybridObject : public HybridObject
    {
    public:
        TestHybridObject() : HybridObject("TestHybridObject") {}

        void loadHybridMethods() override
        {
            registerHybridMethod("setJsCallback", &TestHybridObject::setJsCallback, this);
            registerHybridMethod("callJsCallback", &TestHybridObject::callJsCallback, this);
        }

    private:
        std::function<void()> _jsCallback;
        void setJsCallback(std::function<void()> jsCallback)
        {
            _jsCallback = jsCallback;
        }

        void callJsCallback()
        {
            _jsCallback();
        }
    };
}