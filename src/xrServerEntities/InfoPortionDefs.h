#pragma once

using INFO_DATA = shared_str;
using KNOWN_INFO_VECTOR = xr_vector<INFO_DATA>;

class CFindByIDPred
{
    shared_str element;

public:
    CFindByIDPred(const shared_str& element_to_find) { element = element_to_find; }
    IC bool operator()(const INFO_DATA& data) const { return data == element; }
};
