private:
    BOOL m_DemoPlay = false;
    BOOL m_DemoPlayStarted = false;
    BOOL m_DemoPlayStoped = false;
    BOOL m_DemoSave = false;
    BOOL m_DemoSaveStarted = false;
    u32 m_StartGlobalTime;
    // XXX nitrocaster: why not CurrentControlEntity* ?
    CObject* m_current_spectator = nullptr; // in real, this is CurrentControlEntity 
    message_filter* m_msg_filter = nullptr;
    demoplay_control* m_demoplay_control = nullptr;
public:
#pragma pack(push, 1)
    struct DemoHeader
    {
        u32 m_time_global;
        u32 m_time_server;
        s32 m_time_delta;
        s32 m_time_delta_user;
    };

    struct DemoPacket
    {
        u32 m_time_global_delta;
        u32 m_timeReceive;
        u32 m_packet_size;
        //here will be body of NET_Packet ...
    };
#pragma pack(pop)

    void SetDemoSpectator(CObject* spectator);
    CObject* GetDemoSpectator();
    void PrepareToSaveDemo();
    void SaveDemoInfo();
    inline demo_info* GetDemoInfo() const { return m_demo_info; }
    bool PrepareToPlayDemo(shared_str const & file_name);
    void StartPlayDemo();
    void RestartPlayDemo();
    void StopPlayDemo();
    float GetDemoPlayPos() const;
    //void SetDemoPlayPos(float const pos);
    float GetDemoPlaySpeed() const; //Device.time_factor()
    void SetDemoPlaySpeed(float const time_factor); //Device.time_factor(
    message_filter* GetMessageFilter();
    demoplay_control* GetDemoPlayControl();
    BOOL IsDemoPlay() { return (!m_DemoSave && m_DemoPlay); }
    BOOL IsDemoSave() { return (m_DemoSave && !m_DemoPlay); }
    inline BOOL IsDemoPlayStarted() { return (IsDemoPlay() && m_DemoPlayStarted); }
    inline BOOL IsDemoPlayFinished() { return m_DemoPlayStoped; }
    inline BOOL IsDemoSaveStarted() { return (IsDemoSave() && m_DemoSaveStarted); }
    void SavePacket(NET_Packet& packet);

private:
    void StartSaveDemo(const shared_str& server_options);
    void StopSaveDemo();
    void SpawnDemoSpectator();
    //saving
    void SaveDemoHeader(const shared_str& server_options);
    inline bool IsDemoInfoSaved() { return m_demo_info != nullptr; }
    bool LoadDemoHeader();
    bool LoadPacket(NET_Packet & dest_packet, u32 global_time_delta);
    void SimulateServerUpdate();
    void CatchStartingSpawns();
    void __stdcall MSpawnsCatchCallback(u32 message, u32 subtype, NET_Packet& packet);

    DemoHeader m_demo_header;
    shared_str m_demo_server_options;
    // if instance of this class exist, then the demo info have saved or loaded...
    demo_info* m_demo_info = nullptr;
    u32 m_demo_info_file_pos;
    IWriter* m_writer = nullptr;
    CStreamReader* m_reader = nullptr;
    u32 m_prev_packet_pos;
    u32 m_prev_packet_dtime;
    u32 m_starting_spawns_pos;
    u32 m_starting_spawns_dtime;
