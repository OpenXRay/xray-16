#include "stdafx.h"

#include "recalculation.h"
#include "serialize.h"

void recalculation::load_calculation_params()
{
    IReader* R = FS.r_open("$level$", "do_light.ltx");

    if (!R)
        partial_calculate = false;
    else
    {
        CInifile ini(R);
        if (!ini.section_exist("calculation"))
            partial_calculate = false;
        else
        {
            partial_calculate = !!ini.r_bool("calculation", "calculate_rect");
            force_recalculate = !!ini.r_bool("calculation", "force_recalculate");
            Fvector2 center_rect = ini.r_fvector2("calculation", "center");
            float radius = ini.r_float("calculation", "radius");
            calculation_rect.lt = center_rect;
            calculation_rect.rb = center_rect;
            calculation_rect.grow(radius, radius);
        }
        FS.r_close(R);
    }
}

void recalculation::setup_recalculationflags_file(u32 check_sum)
{
    IWriter* W = FS.w_open("$level$", "recalculation_data_slots.details");
    W->w_chunk(0, &check_sum, sizeof(check_sum));
    // u32 buff_size = dtH.slot_index( dtH.x_size(), dtH.z_size() ) * sizeof( slots_flags[0] );
    u32 buff_size = dtH.slot_count() * sizeof(slots_flags[0]);
    void* buff = xr_alloca(buff_size);
    memset(buff, 0, buff_size);
    W->w_chunk(1, buff, buff_size);
    FS.w_close(W);
}

void recalculation::check_files(u32 check_sum)
{
    string_path N, L;
    FS.update_path(L, "$level$", "level.details");
    FS.update_path(N, "$level$", "recalculation_data_slots.details");
    if (!FS.exist(L))
    {
        FS.file_delete(N);
        return;
    }

    IReader* R = FS.r_open("$level$", "recalculation_data_slots.details");
    if (R)
    {
        u32 check;
        R->r_chunk(0, &check);
        if (check != check_sum)
            FS.file_delete(N);
        else
            recalculate = true;

        FS.r_close(R);
    }
}

void recalculation::check_load(u32 check_sum)
{
    check_files(check_sum);
    string_path N;
    FS.update_path(N, "$level$", "recalculation_data_slots.details");
    if (!FS.exist(N))
        setup_recalculationflags_file(check_sum);
}

void recalculation::load(u32 check_sum)
{
    load_calculation_params();

    check_load(check_sum);

    string_path N;
    FS.update_path(N, "$level$", "recalculation_data_slots.details");

    dtFS = xr_new<CVirtualFileRW>(N);
    dtFS->find_chunk(1);
    slots_flags = (u8*)dtFS->pointer();
}

void recalculation::close()
{
    if (dtFS)
        xr_delete(dtFS);
}

// const DetailHeader				&dtH;
// u8								*slots_flags;
// CVirtualFileRW					*dtFS;

// Frect	calculation_rect;
// bool	recalculate;
// bool	partial_calculate;
// bool	force_recalculate;
void recalculation::read(INetReader& r)
{
    R_ASSERT(!slots_flags);
    R_ASSERT(dtH.version() != u32(-1));
    R_ASSERT(dtH.x_size() != u32(-1));
    R_ASSERT(dtH.z_size() != u32(-1));

    u32 buff_size = dtH.slot_count() * sizeof(slots_flags[0]);
    slots_flags = (u8*)xr_malloc(buff_size);

    r.r(slots_flags, dtH.slot_count());
    r_pod(r, calculation_rect);

    r_pod(r, recalculate);
    r_pod(r, partial_calculate);
    r_pod(r, force_recalculate);
}
void recalculation::write(IWriter& w) const
{
    u32 buff_size = dtH.slot_count() * sizeof(slots_flags[0]);
    w.w(slots_flags, buff_size);
    w_pod(w, calculation_rect);

    w_pod(w, recalculate);
    w_pod(w, partial_calculate);
    w_pod(w, force_recalculate);
}
