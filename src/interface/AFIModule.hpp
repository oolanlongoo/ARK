/*
 * This source file is part of ARK
 * For the latest info, see https://github.com/ArkNX
 *
 * Copyright (c) 2013-2019 ArkNX authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once

#include "base/AFPlatform.hpp"
#include "interface/AFIPluginManager.hpp"

namespace ark {

// class AFIPluginManager;

class AFIModule
{
public:
    AFIModule() = default;
    virtual ~AFIModule() = default;

    virtual bool Init()
    {
        return true;
    }

    virtual bool PostInit()
    {
        return true;
    }

    virtual bool CheckConfig()
    {
        return true;
    }

    virtual bool PreUpdate()
    {
        return true;
    }

    virtual bool Update()
    {
        return true;
    }

    virtual bool PreShut()
    {
        return true;
    }

    virtual bool Shut()
    {
        return true;
    }

    AFIPluginManager* GetPluginManager() const
    {
        return pPluginManager;
    }

    void SetPluginManager(AFIPluginManager* p)
    {
        pPluginManager = p;
    }

    template<typename MODULE>
    MODULE* FindModule()
    {
        return GetPluginManager()->FindModule<MODULE>();
    }

    const std::string& Name() const
    {
        return name_;
    }

    void SetName(const std::string& value)
    {
        name_ = value;
    }

private:
    friend class AFIPluginManager;
    AFIPluginManager* pPluginManager{nullptr};

    std::string name_;
};

} // namespace ark