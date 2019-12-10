#pragma once

#include "alife_space.h"
#include "Common/object_interfaces.h"

struct INFO_DATA : ISerializable
{
    INFO_DATA() :info_id(nullptr), receive_time(0) {};
    INFO_DATA(shared_str id, ALife::_TIME_ID time) : info_id(id), receive_time(time) {};

    void load(IReader& stream) override;
    void save(IWriter&) override;

    shared_str info_id;
    //время получения нужно порции информации
    ALife::_TIME_ID receive_time;
};

class CFindByIDPred
{
    shared_str element;

public:
    CFindByIDPred(const shared_str& element_to_find) { element = element_to_find; }
    bool operator()(const INFO_DATA& data) const { return data.info_id == element; }
};

using KNOWN_INFO_VECTOR = xr_vector<INFO_DATA>;
