/* pad.metal

   Copyright (c) 2003-2024 HandBrake Team

   port of FFmpeg pad.cl.

   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */


#include <metal_stdlib>
#include <metal_integer>
#include <metal_texture>

using namespace metal;

/*
 * Parameters
 */

struct params {
    uint    plane;
    uint    channels;
    float   color_y;
    float   color_u;
    float   color_v;
    uint    x;
    uint    y;
};

/*
 * Texture access helpers
 */

template <typename T>
T tex2D(texture2d<float, access::read> tex, ushort x, ushort y)
{
    return tex.read(ushort2(x, y)).x;
}

template <>
float2 tex2D<float2>(texture2d<float, access::read> tex, ushort x, ushort y)
{
    return tex.read(ushort2(x, y)).xy;
}

/*
 * Pad helpers
 */

template <typename T>
T filter_pixel(
    texture2d<float, access::write> dst,
    texture2d<float, access::read>  src,
    T color,
    ushort2 pos,
    ushort2 src_pos,
    ushort2 offset)
{
    ushort2 src_size = ushort2(src.get_width(), src.get_height());

    T pixel = pos.x >= src_size.x + offset.x ||
              pos.y >= src_size.y + offset.y ||
              pos.x < offset.x ||
              pos.y < offset.y ? color : tex2D<T>(src, src_pos.x, src_pos.y);
    return pixel;
}

/*
 * Kernel dispatch
 */

kernel void pad(
    texture2d<float, access::write> dst [[texture(0)]],
    texture2d<float, access::read>  src [[texture(1)]],
    constant params& p [[buffer(0)]],
    ushort2 pos [[thread_position_in_grid]])
{
    if ((pos.x >= dst.get_width()) || (pos.y >= dst.get_height())) {
        return;
    }

    ushort2 offset = ushort2(p.x, p.y);
    ushort2 src_pos = pos - offset;

    float2 value = p.channels == 1 ?
        float2(filter_pixel<float>(dst, src, p.color_y,
                                   pos, src_pos, offset)) :
        filter_pixel<float2>(dst, src, float2(p.color_v, p.color_u),
                             pos, src_pos, offset);
    dst.write(value.xyyy, pos);
}
