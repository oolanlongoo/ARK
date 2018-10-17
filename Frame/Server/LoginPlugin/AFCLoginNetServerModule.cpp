﻿/*
* This source file is part of ArkGameFrame
* For the latest info, see https://github.com/ArkGame
*
* Copyright (c) 2013-2018 ArkGame authors.
*
* Licensed under the Apache License, Version 2.0 (the "License");
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

#include "AFCLoginNetServerModule.h"

namespace ark
{

    bool AFCLoginNetServerModule::Init()
    {
        m_pLogModule = pPluginManager->FindModule<AFILogModule>();
        m_pBusModule = pPluginManager->FindModule<AFIBusModule>();
        m_pNetServerManagerModule = pPluginManager->FindModule<AFINetServerManagerModule>();
        m_pLoginNetClientModule = pPluginManager->FindModule<AFILoginNetClientModule>();

        return true;
    }

    bool AFCLoginNetServerModule::PostInit()
    {
        int ret = StartServer();
        if (ret != 0)
        {
            exit(0);
            return false;
        }

        return true;
    }

    int AFCLoginNetServerModule::StartServer()
    {
        int ret = m_pNetServerManagerModule->CreateServer();
        if (ret != 0)
        {
            ARK_LOG_ERROR("Cannot start server net, busid = {}, error = {}", m_pBusModule->GetSelfBusName(), ret);
            ARK_ASSERT_NO_EFFECT(0);
            return ret;
        }

        m_pNetServer = m_pNetServerManagerModule->GetSelfNetServer();
        if (m_pNetServer == nullptr)
        {
            ret = -3;
            ARK_LOG_ERROR("Cannot find server net, busid = {}, error = {}", m_pBusModule->GetSelfBusName(), ret);
            return ret;
        }

        m_pNetServer->AddRecvCallback(AFMsg::EGMI_STS_HEART_BEAT, this, &AFCLoginNetServerModule::OnHeartBeat);
        m_pNetServer->AddRecvCallback(AFMsg::EGMI_REQ_LOGIN, this, &AFCLoginNetServerModule::OnLoginProcess);
        m_pNetServer->AddRecvCallback(AFMsg::EGMI_REQ_LOGOUT, this, &AFCLoginNetServerModule::OnLogOut);
        m_pNetServer->AddRecvCallback(AFMsg::EGMI_REQ_CONNECT_WORLD, this, &AFCLoginNetServerModule::OnSelectWorldProcess);
        m_pNetServer->AddRecvCallback(AFMsg::EGMI_REQ_WORLD_LIST, this, &AFCLoginNetServerModule::OnViewWorldProcess);
        m_pNetServer->AddRecvCallback(this, &AFCLoginNetServerModule::InvalidMessage);

        m_pNetServer->AddEventCallBack(this, &AFCLoginNetServerModule::OnSocketClientEvent);

        return 0;
    }

    int AFCLoginNetServerModule::OnSelectWorldResultsProcess(const int nWorldID, const AFGUID xSenderID, const int nLoginID, const std::string& strAccount, const std::string& strWorldURL, const std::string& strWorldKey)
    {
        ARK_SHARE_PTR<AFSessionData> pSessionData = mmClientSessionData.GetElement(xSenderID);

        if (pSessionData != nullptr)
        {
            AFMsg::AckConnectWorldResult xMsg;
            xMsg.set_world_id(nWorldID);
            xMsg.mutable_sender()->CopyFrom(AFIMsgModule::GUIDToPB(xSenderID));
            xMsg.set_login_id(nLoginID);
            xMsg.set_account(strAccount);
            xMsg.set_world_url(strWorldURL);
            xMsg.set_world_key(strWorldKey);

            m_pNetServer->SendPBMsg(AFMsg::EGameMsgID::EGMI_ACK_CONNECT_WORLD, xMsg, pSessionData->mnClientID, xSenderID);
        }

        return 0;
    }

    void AFCLoginNetServerModule::OnClientConnected(const AFGUID& xClientID)
    {
        ARK_SHARE_PTR<AFSessionData> pSessionData = std::make_shared<AFSessionData>();

        pSessionData->mnClientID = xClientID;
        mmClientSessionData.AddElement(xClientID, pSessionData);
    }

    void AFCLoginNetServerModule::OnClientDisconnect(const AFGUID& xClientID)
    {
        mmClientSessionData.RemoveElement(xClientID);
    }

    void AFCLoginNetServerModule::OnLoginProcess(const ARK_PKG_BASE_HEAD& head, const int msg_id, const char* msg, const uint32_t msg_len, const AFGUID& conn_id)
    {
        ARK_PROCESS_MSG(head, msg, msg_len, AFMsg::ReqAccountLogin);
        ARK_SHARE_PTR<AFSessionData> pSession = mmClientSessionData.GetElement(conn_id);

        if (pSession == nullptr)
        {
            return;
        }

        if (pSession->mnLogicState == 0)
        {
            //TODO:will add login process function
            int nState = 0;// m_pLoginLogicModule->OnLoginProcess(pSession->mnClientID, xMsg.account(), xMsg.password());

            if (0 != nState)
            {
                ARK_LOG_ERROR("Check password failed, id = {} account = {} password = {}", conn_id.ToString(), x_msg.account(), x_msg.password());
                AFMsg::AckEventResult xMsg;
                xMsg.set_event_code(AFMsg::EGEC_ACCOUNTPWD_INVALID);

                m_pNetServer->SendPBMsg(AFMsg::EGameMsgID::EGMI_ACK_LOGIN, xMsg, conn_id, actor_id);
                return;
            }

            pSession->mnLogicState = 1;
            pSession->mstrAccout = x_msg.account();

            AFMsg::AckEventResult xData;
            xData.set_event_code(AFMsg::EGEC_ACCOUNT_SUCCESS);
            xData.set_parame1(x_msg.account());
            xData.set_parame2(x_msg.password());
            xData.set_parame3(x_msg.security_code());

            m_pNetServer->SendPBMsg(AFMsg::EGameMsgID::EGMI_ACK_LOGIN, xData, conn_id, actor_id);
            ARK_LOG_INFO("In same scene and group but it not a clone scene, id = {} account = {}", conn_id.ToString(), x_msg.account());
        }
    }

    void AFCLoginNetServerModule::OnSelectWorldProcess(const ARK_PKG_BASE_HEAD& head, const int msg_id, const char* msg, const uint32_t msg_len, const AFGUID& conn_id)
    {
        ARK_PROCESS_MSG(head, msg, msg_len, AFMsg::ReqConnectWorld);
        ARK_SHARE_PTR<AFSessionData> pSession = mmClientSessionData.GetElement(conn_id);

        if (!pSession)
        {
            return;
        }

        //没登录过
        if (pSession->mnLogicState <= 0)
        {
            return;
        }

        AFMsg::ReqConnectWorld xData;
        xData.set_world_id(x_msg.world_id());
        xData.set_login_id(pPluginManager->BusID());
        xData.mutable_sender()->CopyFrom(AFIMsgModule::GUIDToPB(pSession->mnClientID));
        xData.set_account(pSession->mstrAccout);

        //TODO:will fix this
        //m_pLoginToMasterModule->GetClusterModule()->SendSuitByPB(pSession->mstrAccout, AFMsg::EGameMsgID::EGMI_REQ_CONNECT_WORLD, xData, xHead.GetPlayerID());//here has a problem to be solve
    }

    void AFCLoginNetServerModule::OnSocketClientEvent(const NetEventType event, const AFGUID& conn_id, const std::string& ip, int bus_id)
    {
        switch (event)
        {
        case CONNECTED:
            {
                ARK_LOG_INFO("Connected success, id = {}", conn_id.ToString());
                OnClientConnected(conn_id);
            }
            break;
        case DISCONNECTED:
            {
                ARK_LOG_INFO("Connection closed, id = {}", conn_id.ToString());
                OnClientDisconnect(conn_id);
            }
            break;
        default:
            break;
        }
    }

    void AFCLoginNetServerModule::SynWorldToClient(const AFGUID& xClientID)
    {
        AFMsg::AckServerList xData;
        xData.set_type(AFMsg::RSLT_WORLD_SERVER);

        AFMapEx<int, AFMsg::ServerInfoReport>& xWorldMap = m_pLoginNetClientModule->GetWorldMap();

        for (auto pWorldData = xWorldMap.First(); pWorldData != nullptr; pWorldData = xWorldMap.Next())
        {
            AFMsg::ServerInfo* pServerInfo = xData.add_info();

            pServerInfo->set_server_id(pWorldData->bus_id());
            pServerInfo->set_status(pWorldData->logic_status());
            pServerInfo->set_wait_count(0);
        }

        m_pNetServer->SendPBMsg(AFMsg::EGameMsgID::EGMI_ACK_WORLD_LIST, xData, xClientID, AFGUID(0));
    }

    void AFCLoginNetServerModule::OnViewWorldProcess(const ARK_PKG_BASE_HEAD& head, const int msg_id, const char* msg, const uint32_t msg_len, const AFGUID& conn_id)
    {
        ARK_PROCESS_MSG(head, msg, msg_len, AFMsg::ReqServerList);

        if (x_msg.type() == AFMsg::RSLT_WORLD_SERVER)
        {
            SynWorldToClient(conn_id);
        }
    }

    void AFCLoginNetServerModule::OnHeartBeat(const ARK_PKG_BASE_HEAD& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
    {
        //do nothing
    }

    void AFCLoginNetServerModule::OnLogOut(const ARK_PKG_BASE_HEAD& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
    {
        //do nothing
    }

    void AFCLoginNetServerModule::InvalidMessage(const ARK_PKG_BASE_HEAD& xHead, const int nMsgID, const char* msg, const uint32_t nLen, const AFGUID& xClientID)
    {
        ARK_LOG_ERROR("Invalid msg id = {}", nMsgID);
    }

}