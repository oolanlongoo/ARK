// -------------------------------------------------------------------------
//    @FileName         :    AFCGameServerModule.cpp
//    @Author           :    Ark Game Tech
//    @Date             :    2013-01-02
//    @Module           :    AFCGameServerModule
//    @Desc             :
// -------------------------------------------------------------------------

#include "AFGamePlugin.h"
#include "AFCGameServerModule.h"

bool AFCGameServerModule::Init()
{
    m_pKernelModule = pPluginManager->FindModule<AFIKernelModule>();
    m_pClassModule = pPluginManager->FindModule<AFIClassModule>();
    m_pUUIDModule = pPluginManager->FindModule<AFIGUIDModule>();

    m_pUUIDModule->SetWorkerAndDatacenter(1, 1);

    return true;
}

bool AFCGameServerModule::Shut()
{

    return true;
}

bool AFCGameServerModule::Execute()
{
    return true;
}

bool AFCGameServerModule::AfterInit()
{
    return true;
}

bool AFCGameServerModule::BeforeShut()
{
    return true;
}
