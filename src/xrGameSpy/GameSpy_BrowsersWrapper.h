#pragma once
#include "xrGameSpy/xrGameSpy.h"

enum class GSUpdateStatus;
class CGameSpy_Browser;
struct ServerInfo;

/*!
\brief "Accumulator" for responds sequences
\details Use when you need to decide was the sequence of statuses ( for example, from multiple servers) successful or
not
*/
class CGSUpdateStatusAccumulator
{
protected:
    mutable Lock lock;
    xr_vector<GSUpdateStatus> accumulator;

public:
    /*! Creates the accumulator and puts argument as the first status of the series */
    CGSUpdateStatusAccumulator(GSUpdateStatus s);

    /*! Adds the new status to the internal accumulator */
    void Register(GSUpdateStatus s);

    /*! Clears history which is currently stored in the accumulator and stores the first status (from the argument) */
    void Reset(GSUpdateStatus s);

    /*! Returns the generalized status of the stored sequence.
        The sequence succeed when at least one status is "good"
     */
    GSUpdateStatus GetOptimistic() const;

    /*! Returns the generalized status of the stored sequence.
        The sequence fails when at least one status is "bad"
     */
    GSUpdateStatus GetPessimistic() const;

    /*! Indicates whether the system can continue to work or not. */
    static bool IsStatusGood(GSUpdateStatus s);
};

/*!
\brief "Proxy" class which allows support of multiple master-lists in network game.

\details Owns multiple server browsers (one browser for one master-list), accumulates data and presents it as one big
"browser". It has the same interface with CGameSpy_Browser, so we could easily switch between them.
*/
class XRGAMESPY_API CGameSpy_BrowsersWrapper
{
public:
    using UpdateCallback = fastdelegate::FastDelegate<void()>;
    using SubscriberIdx = size_t;

protected:
    struct SBrowserInfo
    {
        xr_unique_ptr<CGameSpy_Browser> browser;
        size_t servers_count;
        bool active;
        bool reportedFailure;
    };
    xr_vector<SBrowserInfo> browsers;

    struct SServerDescription
    {
        CGameSpy_Browser* browser;
        size_t idx;
        void* gs_data;
    };
    xr_vector<SServerDescription> servers;

    // Lock it before accessing 'browsers' or 'servers' members
    Lock servers_lock;

    xr_vector<UpdateCallback> updates_subscriptions;
    // Lock it before accessing 'updates_subscriptions' member
    Lock updates_subscriptions_lock;

    void xr_stdcall UpdateCb(CGameSpy_Browser* gs_browser);
    void ForgetAllServers();

public:
    CGameSpy_BrowsersWrapper();
    ~CGameSpy_BrowsersWrapper();

    /*! \brief Registers a callback for tracking changes in the master-lists.
        \details The delegate from the arguments will be called after any update event in any server-list happens.
        \warning Update event could be fired not only when the new server appears, but also in cases when servers change
       their states.
       \param[in] updateCb delegate which will be called after change happens.
       \return ID of the registered callback, use it in @ref UnsubscribeUpdates
    */
    SubscriberIdx SubscribeUpdates(UpdateCallback updateCb);

    /*! Unsubscribes the user with the specified callback ID and clears the binded delegate
        \param[in] idx callback ID which was returned by @ref SubscribeUpdates
        \return true if unsubscribing was successful, false otherwise
    */
    bool UnsubscribeUpdates(SubscriberIdx idx);

    /*! Clears the internal list of the known servers and starts the process of re-discovering servers.
        \warning Make sure you have cleared variables with IDs of the servers and pointers to internal data! The call of
       the method will invalidate them!
        \return @ref GSUpdateStatus::ConnectingToMaster if the process of network discovery was successfully started
    */
    GSUpdateStatus RefreshList_Full(bool Local, const char* FilterStr = "");

    /*! The method is used to clarify status of the selected server. Use it before obtaining a detail information of the
       game session.
       \param[in] server_id internal ID of the server which you want to operate with
     */
    void RefreshQuick(int server_id);

    bool HasAllKeys(int server_id);

    bool CheckDirectConnection(int server_id);

    /*! Returns the count of currently registeres servers */
    int GetServersCount();

    /*! Fills the structure with actual status of the server
        \param[out] pServerInfo structure which should be filled
        \param[in] server_id internal ID of the server which you want to operate with
    */
    void GetServerInfoByIndex(ServerInfo* pServerInfo, int server_id);

    /*! Provides a "raw" pointer to server data which can be used for further determining custom parameters of the
        server
     */
    void* GetServerByIndex(int server_id);
    bool GetBool(void* srv, int keyId, bool defaultValue = false);
    int GetInt(void* srv, int keyId, int defaultValue = 0);
    float GetFloat(void* srv, int keyId, float defaultValue = 0.0f);

    /*! Performs operations which are necessary for discovering new servers. Please call this method periodically. */
    GSUpdateStatus Update();
};
