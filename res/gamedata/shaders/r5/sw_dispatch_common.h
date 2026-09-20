#ifndef SW_DISPATCH_COMMON_H
#define SW_DISPATCH_COMMON_H

#define SW_DISPATCH_ROW_GROUPS 1024u

uint3 SwDispatchDims(uint groupCount)
{
    uint rows = (groupCount + SW_DISPATCH_ROW_GROUPS - 1u) / SW_DISPATCH_ROW_GROUPS;
    uint columns = (rows > 1u) ? SW_DISPATCH_ROW_GROUPS : groupCount;
    return uint3(columns, rows, 1u);
}

uint SwDispatchLinearGroup(uint3 groupID)
{
    return groupID.x + groupID.y * SW_DISPATCH_ROW_GROUPS;
}

#endif
