/*
 * MP4 library API functions
 *
 * These are wrapper functions that provide C linkage conventions
 * to the library, and catch any internal errors, ensuring that
 * a proper return value is given.
 */

#include "mp4common.h"

static u_int8_t ipod_magic[] = {
 0x6b, 0x68, 0x40, 0xf2, 0x5f, 0x24, 0x4f, 0xc5,
 0xba, 0x39, 0xa5, 0x1b, 0xcf, 0x03, 0x23, 0xf3
};

class IPodUUIDAtom : public MP4Atom {
public:
 IPodUUIDAtom() : MP4Atom("uuid")
 {
 SetExtendedType(ipod_magic);

 MP4Integer32Property* value = new MP4Integer32Property("value");
 value->SetValue(1);
 AddProperty(value);
 }
};

extern "C" void AddIPodUUID(MP4FileHandle hFile, MP4TrackId trackId)
{
 MP4Track* track = ((MP4File*)hFile)->GetTrack(trackId);
 MP4Atom* avc1 = track->GetTrakAtom()->FindChildAtom("mdia.minf.stbl.stsd.avc1");
 avc1->AddChildAtom(new IPodUUIDAtom());
}

