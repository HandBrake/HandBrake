/* dxva2api.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */


#ifndef _DXVA2API_H
#define _DXVA2API_H
#ifdef USE_HWD
#define MINGW_DXVA2API_H_VERSION (2)

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#include <objbase.h>
#include <d3d9.h>

/* Define it to allow using nameless struct/union (non C99 compliant) to match
 * the official documentation. */
//#define DXVA2API_USE_BITFIELDS

/****************STRUCTURES******************/
#pragma pack(push, 1)

#define DXVA2API_USE_BITFIELDS

typedef struct _DXVA2_ExtendedFormat {
#ifdef DXVA2API_USE_BITFIELDS
    union {
        struct {
            UINT SampleFormat           : 8;
            UINT VideoChromaSubsampling : 4;
            UINT NominalRange           : 3;
            UINT VideoTransferMatrix    : 3;
            UINT VideoLighting          : 4;
            UINT VideoPrimaries         : 5;
            UINT VideoTransferFunction  : 5;
        };
        UINT value;
    };
#else
    UINT value;
#endif
} DXVA2_ExtendedFormat;

typedef struct _DXVA2_Frequency {
    UINT Numerator;
    UINT Denominator;
} DXVA2_Frequency;

typedef struct _DXVA2_VideoDesc {
    UINT SampleWidth;
    UINT SampleHeight;
    DXVA2_ExtendedFormat SampleFormat;
    D3DFORMAT Format;
    DXVA2_Frequency InputSampleFreq;
    DXVA2_Frequency OutputFrameFreq;
    UINT UABProtectionLevel;
    UINT Reserved;
} DXVA2_VideoDesc;

typedef struct _DXVA2_ConfigPictureDecode {
    GUID guidConfigBitstreamEncryption;
    GUID guidConfigMBcontrolEncryption;
    GUID guidConfigResidDiffEncryption;
    UINT ConfigBitstreamRaw;
    UINT ConfigMBcontrolRasterOrder;
    UINT ConfigResidDiffHost;
    UINT ConfigSpatialResid8;
    UINT ConfigResid8Subtraction;
    UINT ConfigSpatialHost8or9Clipping;
    UINT ConfigSpatialResidInterleaved;
    UINT ConfigIntraResidUnsigned;
    UINT ConfigResidDiffAccelerator;
    UINT ConfigHostInverseScan;
    UINT ConfigSpecificIDCT;
    UINT Config4GroupedCoefs;
    USHORT ConfigMinRenderTargetBuffCount;
    USHORT ConfigDecoderSpecific;
} DXVA2_ConfigPictureDecode;

typedef struct _DXVA2_DecodeBufferDesc {
    DWORD CompressedBufferType;
    UINT BufferIndex;
    UINT DataOffset;
    UINT DataSize;
    UINT FirstMBaddress;
    UINT NumMBsInBuffer;
    UINT Width;
    UINT Height;
    UINT Stride;
    UINT ReservedBits;
    PVOID pvPVPState;
} DXVA2_DecodeBufferDesc;

typedef struct _DXVA2_DecodeExtensionData {
    UINT Function;
    PVOID pPrivateInputData;
    UINT PrivateInputDataSize;
    PVOID pPrivateOutputData;
    UINT PrivateOutputDataSize;
} DXVA2_DecodeExtensionData;

typedef struct _DXVA2_DecodeExecuteParams {
    UINT NumCompBuffers;
    DXVA2_DecodeBufferDesc *pCompressedBuffers;
    DXVA2_DecodeExtensionData *pExtensionData;
} DXVA2_DecodeExecuteParams;

enum {
    DXVA2_VideoDecoderRenderTarget  = 0,
    DXVA2_VideoProcessorRenderTarget= 1,
    DXVA2_VideoSoftwareRenderTarget = 2
};

enum {
    DXVA2_PictureParametersBufferType   = 0,
    DXVA2_MacroBlockControlBufferType   = 1,
    DXVA2_ResidualDifferenceBufferType  = 2,
    DXVA2_DeblockingControlBufferType   = 3,
    DXVA2_InverseQuantizationMatrixBufferType   = 4,
    DXVA2_SliceControlBufferType    = 5,
    DXVA2_BitStreamDateBufferType   = 6,
    DXVA2_MotionVectorBuffer        = 7,
    DXVA2_FilmGrainBuffer           = 8
};

/* DXVA MPEG-I/II and VC-1 */
typedef struct _DXVA_PictureParameters {
    USHORT  wDecodedPictureIndex;
    USHORT  wDeblockedPictureIndex;
    USHORT  wForwardRefPictureIndex;
    USHORT  wBackwardRefPictureIndex;
    USHORT  wPicWidthInMBminus1;
    USHORT  wPicHeightInMBminus1;
    UCHAR   bMacroblockWidthMinus1;
    UCHAR   bMacroblockHeightMinus1;
    UCHAR   bBlockWidthMinus1;
    UCHAR   bBlockHeightMinus1;
    UCHAR   bBPPminus1;
    UCHAR   bPicStructure;
    UCHAR   bSecondField;
    UCHAR   bPicIntra;
    UCHAR   bPicBackwardPrediction;
    UCHAR   bBidirectionalAveragingMode;
    UCHAR   bMVprecisionAndChromaRelation;
    UCHAR   bChromaFormat;
    UCHAR   bPicScanFixed;
    UCHAR   bPicScanMethod;
    UCHAR   bPicReadbackRequests;
    UCHAR   bRcontrol;
    UCHAR   bPicSpatialResid8;
    UCHAR   bPicOverflowBlocks;
    UCHAR   bPicExtrapolation;
    UCHAR   bPicDeblocked;
    UCHAR   bPicDeblockConfined;
    UCHAR   bPic4MVallowed;
    UCHAR   bPicOBMC;
    UCHAR   bPicBinPB;
    UCHAR   bMV_RPS;
    UCHAR   bReservedBits;
    USHORT  wBitstreamFcodes;
    USHORT  wBitstreamPCEelements;
    UCHAR   bBitstreamConcealmentNeed;
    UCHAR   bBitstreamConcealmentMethod;
} DXVA_PictureParameters, *LPDXVA_PictureParameters;

typedef struct _DXVA_QmatrixData {
    BYTE    bNewQmatrix[4];
    WORD    Qmatrix[4][8 * 8];
} DXVA_QmatrixData, *LPDXVA_QmatrixData;

typedef struct _DXVA_SliceInfo {
    USHORT  wHorizontalPosition;
    USHORT  wVerticalPosition;
    UINT    dwSliceBitsInBuffer;
    UINT    dwSliceDataLocation;
    UCHAR   bStartCodeBitOffset;
    UCHAR   bReservedBits;
    USHORT  wMBbitOffset;
    USHORT  wNumberMBsInSlice;
    USHORT  wQuantizerScaleCode;
    USHORT  wBadSliceChopping;
} DXVA_SliceInfo, *LPDXVA_SliceInfo;

/* DXVA H264 */
typedef struct {
#ifdef DXVA2API_USE_BITFIELDS
    union {
        struct {
            UCHAR Index7Bits     : 7;
            UCHAR AssociatedFlag : 1;
        };
        UCHAR bPicEntry;
    };
#else
    UCHAR bPicEntry;
#endif
} DXVA_PicEntry_H264;


typedef struct {
    USHORT wFrameWidthInMbsMinus1;
    USHORT wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264 CurrPic;
    UCHAR  num_ref_frames;
#ifdef DXVA2API_USE_BITFIELDS
    union {
        struct {
            USHORT field_pic_flag           : 1;
            USHORT MbaffFrameFlag           : 1;
            USHORT residual_colour_transform_flag : 1;
            USHORT sp_for_switch_flag       : 1;
            USHORT chroma_format_idc        : 2;
            USHORT RefPicFlag               : 1;
            USHORT constrained_intra_pred_flag : 1;
            USHORT weighted_pred_flag       : 1;
            USHORT weighted_bipred_idc      : 2;
            USHORT MbsConsecutiveFlag       : 1;
            USHORT frame_mbs_only_flag      : 1;
            USHORT transform_8x8_mode_flag  : 1;
            USHORT MinLumaBipredSize8x8Flag : 1;
            USHORT IntraPicFlag             : 1;
        };
        USHORT wBitFields;
    };
#else
    USHORT wBitFields;
#endif
    UCHAR   bit_depth_luma_minus8;
    UCHAR   bit_depth_chroma_minus8;
    USHORT  Reserved16Bits;
    UINT    StatusReportFeedbackNumber;
    DXVA_PicEntry_H264 RefFrameList[16];
    INT     CurrFieldOrderCnt[2];
    INT     FieldOrderCntList[16][2];
    CHAR    pic_init_qs_minus26;
    CHAR    chroma_qp_index_offset;
    CHAR    second_chroma_qp_index_offset;
    UCHAR   ContinuationFlag;
    CHAR    pic_init_qp_minus26;
    UCHAR   num_ref_idx_l0_active_minus1;
    UCHAR   num_ref_idx_l1_active_minus1;
    UCHAR   Reserved8BitsA;
    USHORT  FrameNumList[16];

    UINT    UsedForReferenceFlags;
    USHORT  NonExistingFrameFlags;
    USHORT  frame_num;
    UCHAR   log2_max_frame_num_minus4;
    UCHAR   pic_order_cnt_type;
    UCHAR   log2_max_pic_order_cnt_lsb_minus4;
    UCHAR   delta_pic_order_always_zero_flag;
    UCHAR   direct_8x8_inference_flag;
    UCHAR   entropy_coding_mode_flag;
    UCHAR   pic_order_present_flag;
    UCHAR   num_slice_groups_minus1;
    UCHAR   slice_group_map_type;
    UCHAR   deblocking_filter_control_present_flag;
    UCHAR   redundant_pic_cnt_present_flag;
    UCHAR   Reserved8BitsB;
    USHORT  slice_group_change_rate_minus1;
    UCHAR   SliceGroupMap[810];
} DXVA_PicParams_H264;

typedef struct {
    UCHAR   bScalingLists4x4[6][16];
    UCHAR   bScalingLists8x8[2][64];
} DXVA_Qmatrix_H264;


typedef struct {
    UINT    BSNALunitDataLocation;
    UINT    SliceBytesInBuffer;
    USHORT  wBadSliceChopping;
    USHORT  first_mb_in_slice;
    USHORT  NumMbsForSlice;
    USHORT  BitOffsetToSliceData;
    UCHAR   slice_type;
    UCHAR   luma_log2_weight_denom;
    UCHAR   chroma_log2_weight_denom;

    UCHAR   num_ref_idx_l0_active_minus1;
    UCHAR   num_ref_idx_l1_active_minus1;
    CHAR    slice_alpha_c0_offset_div2;
    CHAR    slice_beta_offset_div2;
    UCHAR   Reserved8Bits;
    DXVA_PicEntry_H264 RefPicList[2][32];
    SHORT   Weights[2][32][3][2];
    CHAR    slice_qs_delta;
    CHAR    slice_qp_delta;
    UCHAR   redundant_pic_cnt;
    UCHAR   direct_spatial_mv_pred_flag;
    UCHAR   cabac_init_idc;
    UCHAR   disable_deblocking_filter_idc;
    USHORT  slice_id;
} DXVA_Slice_H264_Long;

typedef struct {
    UINT    BSNALunitDataLocation;
    UINT    SliceBytesInBuffer;
    USHORT  wBadSliceChopping;
} DXVA_Slice_H264_Short;

typedef struct {
    USHORT  wFrameWidthInMbsMinus1;
    USHORT  wFrameHeightInMbsMinus1;
    DXVA_PicEntry_H264 InPic;
    DXVA_PicEntry_H264 OutPic;
    USHORT  PicOrderCnt_offset;
    INT     CurrPicOrderCnt;
    UINT    StatusReportFeedbackNumber;
    UCHAR   model_id;
    UCHAR   separate_colour_description_present_flag;
    UCHAR   film_grain_bit_depth_luma_minus8;
    UCHAR   film_grain_bit_depth_chroma_minus8;
    UCHAR   film_grain_full_range_flag;
    UCHAR   film_grain_colour_primaries;
    UCHAR   film_grain_transfer_characteristics;
    UCHAR   film_grain_matrix_coefficients;
    UCHAR   blending_mode_id;
    UCHAR   log2_scale_factor;
    UCHAR   comp_model_present_flag[4];
    UCHAR   num_intensity_intervals_minus1[4];
    UCHAR   num_model_values_minus1[4];
    UCHAR   intensity_interval_lower_bound[3][16];
    UCHAR   intensity_interval_upper_bound[3][16];
    SHORT   comp_model_value[3][16][8];
} DXVA_FilmGrainChar_H264;

typedef struct {
    union {
        struct {
            USHORT  Fraction;
            SHORT   Value;
        };
        LONG ll;
    };
}DXVA2_Fixed32;

typedef struct {
    UCHAR Cr;
    UCHAR Cb;
    UCHAR Y;
    UCHAR Alpha;
}DXVA2_AYUVSample8;

typedef struct {
    USHORT Cr;
    USHORT Cb;
    USHORT Y;
    USHORT Alpha;
}DXVA2_AYUVSample16;

typedef struct {
    DXVA2_Fixed32 MinValue;
    DXVA2_Fixed32 MaxValue;
    DXVA2_Fixed32 DefaultValue;
    DXVA2_Fixed32 StepSize;
}DXVA2_ValueRange;

typedef struct {
    DXVA2_Fixed32 Brightness;
    DXVA2_Fixed32 Contrast;
    DXVA2_Fixed32 Hue;
    DXVA2_Fixed32 Saturation;
}DXVA2_ProcAmpValues;

typedef struct {
    DXVA2_Fixed32 Level;
    DXVA2_Fixed32 Threshold;
    DXVA2_Fixed32 Radius;
}DXVA2_FilterValues;

typedef struct {
    UINT DeviceCaps;
    D3DPOOL InputPool;
    UINT NumForwardRefSamples;
    UINT NumBackwardRefSamples;
    UINT Reserved;
    UINT DeinterlaceTechnology;
    UINT ProcAmpControlCaps;
    UINT VideoProcessorOperations;
    UINT NoiseFilterTechnology;
    UINT DetailFilterTechnology;
}DXVA2_VideoProcessorCaps;

#ifndef _REFERENCE_TIME_
#define _REFERENCE_TIME_
typedef long long int64_t;
typedef int64_t REFERENCE_TIME;
#endif

typedef struct {
    REFERENCE_TIME Start;
    REFERENCE_TIME End;
    DXVA2_ExtendedFormat SampleFormat;
    IDirect3DSurface9 *SrcSurface;
    RECT SrcRect;
    RECT DstRect;
    DXVA2_AYUVSample8 Pal[16];
    DXVA2_Fixed32 PlanarAlpha;
    DWORD SampleData;
}DXVA2_VideoSample;


typedef struct {
    REFERENCE_TIME TargetFrame;
    RECT TargetRect;
    SIZE ConstrictionSize;
    UINT StreamingFlags;
    DXVA2_AYUVSample16 BackgroundColor;
    DXVA2_ExtendedFormat DestFormat;
    DXVA2_ProcAmpValues ProcAmpValues;
    DXVA2_Fixed32 Alpha;
    DXVA2_FilterValues NoiseFilterLuma;
    DXVA2_FilterValues NoiseFilterChroma;
    DXVA2_FilterValues DetailFilterLuma;
    DXVA2_FilterValues DetailFilterChroma;
    DWORD DestData;
} DXVA2_VideoProcessBltParams;

#pragma pack(pop)

/*************INTERFACES************/
#ifdef __cplusplus
extern "C" {
#endif
#define _COM_interface struct
typedef _COM_interface IDirectXVideoDecoderService IDirectXVideoDecoderService;
typedef _COM_interface IDirectXVideoDecoder IDirectXVideoDecoder;

#undef INTERFACE
#define INTERFACE IDirectXVideoDecoder
DECLARE_INTERFACE_( IDirectXVideoDecoder, IUnknown )
{
    STDMETHOD( QueryInterface ) ( THIS_ REFIID, PVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef ) ( THIS ) PURE;
    STDMETHOD_( ULONG, Release ) ( THIS ) PURE;
    STDMETHOD( GetVideoDecoderService ) ( THIS_ IDirectXVideoDecoderService** ) PURE;
    STDMETHOD( GetCreationParameters ) ( THIS_ GUID*, DXVA2_VideoDesc*, DXVA2_ConfigPictureDecode*, IDirect3DSurface9***, UINT* ) PURE;
    STDMETHOD( GetBuffer ) ( THIS_ UINT, void**, UINT* ) PURE;
    STDMETHOD( ReleaseBuffer ) ( THIS_ UINT ) PURE;
    STDMETHOD( BeginFrame ) ( THIS_ IDirect3DSurface9 *, void* ) PURE;
    STDMETHOD( EndFrame ) ( THIS_ HANDLE * ) PURE;
    STDMETHOD( Execute ) ( THIS_ const DXVA2_DecodeExecuteParams* ) PURE;


};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoDecoder_QueryInterface( p, a, b )  (p)->lpVtbl->QueryInterface( p, a, b )
#define IDirectXVideoDecoder_AddRef( p )  (p)->lpVtbl->AddRef( p )
#define IDirectXVideoDecoder_Release( p ) (p)->lpVtbl->Release( p )
#define IDirectXVideoDecoder_BeginFrame( p, a, b )  (p)->lpVtbl->BeginFrame( p, a, b )
#define IDirectXVideoDecoder_EndFrame( p, a )  (p)->lpVtbl->EndFrame( p, a )
#define IDirectXVideoDecoder_Execute( p, a )   (p)->lpVtbl->Execute( p, a )
#define IDirectXVideoDecoder_GetBuffer( p, a, b, c ) (p)->lpVtbl->GetBuffer( p, a, b, c )
#define IDirectXVideoDecoder_GetCreationParameters( p, a, b, c, d, e ) (p)->lpVtbl->GetCreationParameters( p, a, b, c, d, e )
#define IDirectXVideoDecoder_GetVideoDecoderService( p, a )    (p)->lpVtbl->GetVideoDecoderService( p, a )
#define IDirectXVideoDecoder_ReleaseBuffer( p, a ) (p)->lpVtbl->ReleaseBuffer( p, a )
#else
#define IDirectXVideoDecoder_QueryInterface( p, a, b )  (p)->QueryInterface( a, b )
#define IDirectXVideoDecoder_AddRef( p )  (p)->AddRef()
#define IDirectXVideoDecoder_Release( p ) (p)->Release()
#define IDirectXVideoDecoder_BeginFrame( p, a, b )  (p)->BeginFrame( a, b )
#define IDirectXVideoDecoder_EndFrame( p, a )  (p)->EndFrame( a )
#define IDirectXVideoDecoder_Execute( p, a )   (p)->Execute( a )
#define IDirectXVideoDecoder_GetBuffer( p, a, b, c ) (p)->GetBuffer( a, b, c )
#define IDirectXVideoDecoder_GetCreationParameters( p, a, b, c, d, e ) (p)->GetCreationParameters( a, b, c, d, e )
#define IDirectXVideoDecoder_GetVideoDecoderService( p, a )    (p)->GetVideoDecoderService( a )
#define IDirectXVideoDecoder_ReleaseBuffer( p, a ) (p)->ReleaseBuffer( a )
#endif

#undef INTERFACE
#define INTERFACE IDirectXVideoAccelerationService
DECLARE_INTERFACE_( IDirectXVideoAccelerationService, IUnknown )
{
    STDMETHOD( QueryInterface ) ( THIS_ REFIID, PVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef ) ( THIS ) PURE;
    STDMETHOD_( ULONG, Release ) ( THIS ) PURE;
    STDMETHOD( CreateSurface ) ( THIS_ UINT, UINT, UINT, D3DFORMAT, D3DPOOL, DWORD, DWORD, IDirect3DSurface9**, HANDLE* ) PURE;

};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoAccelerationService_QueryInterface( p, a, b )  (p)->lpVtbl->QueryInterface( p, a, b )
#define IDirectXVideoAccelerationService_AddRef( p )  (p)->lpVtbl->AddRef( p )
#define IDirectXVideoAccelerationService_Release( p ) (p)->lpVtbl->Release( p )
#define IDirectXVideoAccelerationService_CreateSurface( p, a, b, c, d, e, f, g, h, i ) (p)->lpVtbl->CreateSurface( p, a, b, c, d, e, f, g, h, i )
#else
#define IDirectXVideoAccelerationService_QueryInterface( p, a, b )  (p)->QueryInterface( a, b )
#define IDirectXVideoAccelerationService_AddRef( p )  (p)->AddRef()
#define IDirectXVideoAccelerationService_Release( p ) (p)->Release()
#define IDirectXVideoAccelerationService_CreateSurface( p, a, b, c, d, e, f, g, h, i ) (p)->CreateSurface( a, b, c, d, e, f, g, h, i )
#endif

#undef INTERFACE
#define INTERFACE IDirectXVideoDecoderService
DECLARE_INTERFACE_( IDirectXVideoDecoderService, IDirectXVideoAccelerationService )
{
    STDMETHOD( QueryInterface ) ( THIS_ REFIID, PVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef ) ( THIS ) PURE;
    STDMETHOD_( ULONG, Release ) ( THIS ) PURE;
    STDMETHOD( CreateSurface ) ( THIS_ UINT, UINT, UINT, D3DFORMAT, D3DPOOL, DWORD, DWORD, IDirect3DSurface9**, HANDLE* ) PURE;
    STDMETHOD( GetDecoderDeviceGuids ) ( THIS_ UINT*, GUID ** ) PURE;
    STDMETHOD( GetDecoderRenderTargets ) ( THIS_ REFGUID, UINT*, D3DFORMAT** ) PURE;
    STDMETHOD( GetDecoderConfigurations ) ( THIS_ REFGUID, const DXVA2_VideoDesc*, IUnknown*, UINT*, DXVA2_ConfigPictureDecode** ) PURE;
    STDMETHOD( CreateVideoDecoder ) ( THIS_ REFGUID, const DXVA2_VideoDesc*, DXVA2_ConfigPictureDecode*, IDirect3DSurface9**, UINT, IDirectXVideoDecoder** ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoDecoderService_QueryInterface( p, a, b )   (p)->lpVtbl->QueryInterface( p, a, b )
#define IDirectXVideoDecoderService_AddRef( p )   (p)->lpVtbl->AddRef( p )
#define IDirectXVideoDecoderService_Release( p )  (p)->lpVtbl->Release( p )
#define IDirectXVideoDecoderService_CreateSurface( p, a, b, c, d, e, f, g, h, i )  (p)->lpVtbl->CreateSurface( p, a, b, c, d, e, f, g, h, i )
#define IDirectXVideoDecoderService_CreateVideoDecoder( p, a, b, c, d, e, f )   (p)->lpVtbl->CreateVideoDecoder( p, a, b, c, d, e, f )
#define IDirectXVideoDecoderService_GetDecoderConfigurations( p, a, b, c, d, e )   (p)->lpVtbl->GetDecoderConfigurations( p, a, b, c, d, e )
#define IDirectXVideoDecoderService_GetDecoderDeviceGuids( p, a, b )    (p)->lpVtbl->GetDecoderDeviceGuids( p, a, b )
#define IDirectXVideoDecoderService_GetDecoderRenderTargets( p, a, b, c )    (p)->lpVtbl->GetDecoderRenderTargets( p, a, b, c )
#else
#define IDirectXVideoDecoderService_QueryInterface( p, a, b )   (p)->QueryInterface( a, b )
#define IDirectXVideoDecoderService_AddRef( p )   (p)->AddRef()
#define IDirectXVideoDecoderService_Release( p )  (p)->Release()
#define IDirectXVideoDecoderService_CreateSurface( p, a, b, c, d, e, f, g, h, i )  (p)->CreateSurface( a, b, c, d, e, f, g, h, i )
#define IDirectXVideoDecoderService_CreateVideoDecoder( p, a, b, c, d, e, f )   (p)->CreateVideoDecoder( a, b, c, d, e, f )
#define IDirectXVideoDecoderService_GetDecoderConfigurations( p, a, b, c, d, e )   (p)->GetDecoderConfigurations( a, b, c, d, e )
#define IDirectXVideoDecoderService_GetDecoderDeviceGuids( p, a, b )    (p)->GetDecoderDeviceGuids( a, b )
#define IDirectXVideoDecoderService_GetDecoderRenderTargets( p, a, b, c )    (p)->GetDecoderRenderTargets( a, b, c )
#endif

#undef INTERFACE
#define INTERFACE IDirect3DDeviceManager9
DECLARE_INTERFACE_( IDirect3DDeviceManager9, IUnknown )
{
    STDMETHOD( QueryInterface ) ( THIS_ REFIID, PVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef ) ( THIS ) PURE;
    STDMETHOD_( ULONG, Release ) ( THIS ) PURE;
    STDMETHOD( ResetDevice ) ( THIS_ IDirect3DDevice9*, UINT ) PURE;
    STDMETHOD( OpenDeviceHandle ) ( THIS_ HANDLE* ) PURE;
    STDMETHOD( CloseDeviceHandle ) ( THIS_ HANDLE ) PURE;
    STDMETHOD( TestDevice ) ( THIS_ HANDLE ) PURE;
    STDMETHOD( LockDevice ) ( THIS_ HANDLE, IDirect3DDevice9**, BOOL ) PURE;
    STDMETHOD( UnlockDevice ) ( THIS_ HANDLE, BOOL ) PURE;
    STDMETHOD( GetVideoService ) ( THIS_ HANDLE, REFIID, void** ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirect3DDeviceManager9_QueryInterface( p, a, b )   (p)->lpVtbl->QueryInterface( p, a, b )
#define IDirect3DDeviceManager9_AddRef( p )   (p)->lpVtbl->AddRef( p )
#define IDirect3DDeviceManager9_Release( p )  (p)->lpVtbl->Release( p )
#define IDirect3DDeviceManager9_ResetDevice( p, a, b ) (p)->lpVtbl->ResetDevice( p, a, b )
#define IDirect3DDeviceManager9_OpenDeviceHandle( p, a ) (p)->lpVtbl->OpenDeviceHandle( p, a )
#define IDirect3DDeviceManager9_CloseDeviceHandle( p, a ) (p)->lpVtbl->CloseDeviceHandle( p, a )
#define IDirect3DDeviceManager9_TestDevice( p, a ) (p)->lpVtbl->TestDevice( p, a )
#define IDirect3DDeviceManager9_LockDevice( p, a, b, c ) (p)->lpVtbl->LockDevice( p, a, b, c )
#define IDirect3DDeviceManager9_UnlockDevice( p, a, b ) (p)->lpVtbl->UnlockDevice( p, a, b )
#define IDirect3DDeviceManager9_GetVideoService( p, a, b, c ) (p)->lpVtbl->GetVideoService( p, a, b, c )
#else
#define IDirect3DDeviceManager9_QueryInterface( p, a, b )   (p)->QueryInterface( a, b )
#define IDirect3DDeviceManager9_AddRef( p )   (p)->AddRef()
#define IDirect3DDeviceManager9_Release( p )  (p)->Release()
#define IDirect3DDeviceManager9_ResetDevice( p, a, b ) (p)->ResetDevice( a, b )
#define IDirect3DDeviceManager9_OpenDeviceHandle( p, a ) (p)->OpenDeviceHandle( a )
#define IDirect3DDeviceManager9_CloseDeviceHandle( p, a ) (p)->CloseDeviceHandle( a )
#define IDirect3DDeviceManager9_TestDevice( p, a ) (p)->TestDevice( a )
#define IDirect3DDeviceManager9_LockDevice( p, a, b, c ) (p)->LockDevice( a, b, c )
#define IDirect3DDeviceManager9_UnlockDevice( p, a, b ) (p)->UnlockDevice( a, b )
#define IDirect3DDeviceManager9_GetVideoService( p, a, b, c ) (p)->GetVideoService( a, b, c )
#endif

typedef _COM_interface IDirectXVideoProcessorService IDirectXVideoProcessorService;
typedef _COM_interface IDirectXVideoProcessor IDirectXVideoProcessor;

#undef INTERFACE
#define INTERFACE IDirectXVideoProcessor
DECLARE_INTERFACE_( IDirectXVideoProcessor, IUnknown )
{
    STDMETHOD( QueryInterface ) ( THIS_ REFIID, PVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef ) ( THIS ) PURE;
    STDMETHOD_( ULONG, Release ) ( THIS ) PURE;
    STDMETHOD( GetVideoProcessorService ) ( THIS_ IDirectXVideoProcessorService** ) PURE;
    STDMETHOD( GetCreationParameters ) ( THIS_ GUID*, DXVA2_VideoDesc*, D3DFORMAT*, UINT* ) PURE;
    STDMETHOD( GetVideoProcessorCaps ) ( THIS_ DXVA2_VideoProcessorCaps* ) PURE;
    STDMETHOD( GetProcAmpRange ) ( THIS_ UINT, DXVA2_ValueRange* ) PURE;
    STDMETHOD( GetFilterPropertyRange ) ( THIS_ UINT, DXVA2_ValueRange* ) PURE;
    STDMETHOD( VideoProcessBlt ) ( THIS_ IDirect3DSurface9*, DXVA2_VideoProcessBltParams*, DXVA2_VideoSample*, UINT, HANDLE* ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoProcessor_QueryInterface( p, a, b ) (p)->lpVtbl->QueryInterface( p, a, b )
#define IDirectXVideoProcessor_AddRef( p )  (p)->lpVtbl->AddRef( p )
#define IDirectXVideoProcessor_Release( p ) (p)->lpVtbl->Release( p )
#define IDirectXVideoProcessor_GetVideoProcessorService( p, a ) (p)->lpVtbl->GetVideoProcessorService( p, a )
#define IDirectXVideoProcessor_GetCreationParameters( p, a, b, c, d ) (p)->lpVtbl->GetCreationParameters( p, a, b, c, d )
#define IDirectXVideoProcessor_GetVideoProcessorCaps( p, a ) (p)->lpVtbl->GetVideoProcessorCaps( p, a )
#define IDirectXVideoProcessor_GetProcAmpRange( p, a, b ) (p)->lpVtbl->GetProcAmpRange( p, a, b )
#define IDirectXVideoProcessor_GetFilterPropertyRange( p, a, b ) (p)->lpVtbl->GetFilterPropertyRange( p, a, b )
#define IDirectXVideoProcessor_VideoProcessBlt( p, a, b, c, d, e ) (p)->lpVtbl->VideoProcessBlt( p, a, b, c, d, e )
#else
#define IDirectXVideoProcessor_QueryInterface( p, a, b ) (p)->QueryInterface( a, b )
#define IDirectXVideoProcessor_AddRef( p )  (p)->AddRef()
#define IDirectXVideoProcessor_Release( p ) (p)->Release()
#define IDirectXVideoProcessor_GetVideoProcessorService( p, a ) (p)->GetVideoProcessorService( a )
#define IDirectXVideoProcessor_GetCreationParameters( p, a, b, c, d ) (p)->GetCreationParameters( a, b, c, d )
#define IDirectXVideoProcessor_GetVideoProcessorCaps( p, a ) (p)->GetVideoProcessorCaps( a )
#define IDirectXVideoProcessor_GetProcAmpRange( p, a, b ) (p)->GetProcAmpRange( a, b )
#define IDirectXVideoProcessor_GetFilterPropertyRange( p, a, b ) (p)->GetFilterPropertyRange( a, b )
#define IDirectXVideoProcessor_VideoProcessBlt( p, a, b, c, d, e ) (p)->VideoProcessBlt( a, b, c, d, e )
#endif


#undef INTERFACE
#define INTERFACE IDirectXVideoProcessorService
DECLARE_INTERFACE_( IDirectXVideoProcessorService, IDirectXVideoAccelerationService )
{
    STDMETHOD( QueryInterface ) ( THIS_ REFIID, PVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef ) ( THIS ) PURE;
    STDMETHOD_( ULONG, Release ) ( THIS ) PURE;
    STDMETHOD( CreateSurface ) ( THIS_ UINT, UINT, UINT, D3DFORMAT, D3DPOOL, DWORD, DWORD, IDirect3DSurface9**, HANDLE* ) PURE;
    STDMETHOD( RegisterVideoProcessorSoftwareDevice ) ( THIS_ void* ) PURE;
    STDMETHOD( GetVideoProcessorDeviceGuids ) ( THIS_ DXVA2_VideoDesc*, UINT, GUID** ) PURE;
    STDMETHOD( GetVideoProcessorRenderTargets ) ( THIS_ REFGUID, DXVA2_VideoDesc*, UINT*, D3DFORMAT** ) PURE;
    STDMETHOD( GetVideoProcessorSubStreamFormats ) ( THIS_ REFGUID, DXVA2_VideoDesc*, D3DFORMAT, UINT*, D3DFORMAT** ) PURE;
    STDMETHOD( GetVideoProcessorCaps ) ( THIS_ REFGUID, DXVA2_VideoDesc*, D3DFORMAT, DXVA2_VideoProcessorCaps* ) PURE;
    STDMETHOD( GetProcAmpRange ) ( THIS_ REFGUID, DXVA2_VideoDesc*, D3DFORMAT, UINT, DXVA2_ValueRange* ) PURE;
    STDMETHOD( GetFilterPropertyRange ) ( THIS_ REFGUID, DXVA2_VideoDesc*, D3DFORMAT, UINT, DXVA2_ValueRange* ) PURE;
    STDMETHOD( CreateVideoProcessor ) ( THIS_ REFGUID, DXVA2_VideoDesc*, D3DFORMAT, UINT, IDirectXVideoProcessor** ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoProcessorService_QueryInterface( p, a, b ) (p)->lpVtbl->QueryInterface( p, a, b )
#define IDirectXVideoProcessorService_AddRef( p )  (p)->lpVtbl->AddRef( p )
#define IDirectXVideoProcessorService_Release( p ) (p)->lpVtbl->Release( p )
#define IDirectXVideoProcessorService_CreateSurface( p, a, b, c, d, e, f, g, h, i ) (p)->lpVtbl->CreateSurface( p, a, b, c, d, e, f, g, h, i )
#define IDirectXVideoProcessorService_RegisterVideoProcessorSoftwareDevice( p, a ) (p)->lpVtbl->RegisterVideoProcessorSoftwareDevice( p, a )
#define IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids( p, a, b, c ) (p)->lpVtbl->GetVideoProcessorDeviceGuids( p, a, b, c )
#define IDirectXVideoProcessorService_GetVideoProcessorRenderTargets( p, a, b, c, d ) (p)->lpVtbl->GetVideoProcessorRenderTargets( p, a, b, c, d )
#define IDirectXVideoProcessorService_GetVideoProcessorSubStreamFormats( p, a, b, c, d, e ) (p)->lpVtbl->GetVideoProcessorSubStreamFormats( p, a, b, c, d, e )
#define IDirectXVideoProcessorService_GetVideoProcessorCaps( p, a, b, c, d ) (p)->lpVtbl->GetVideoProcessorCaps( p, a, b, c, d )
#define IDirectXVideoProcessorService_GetProcAmpRange( p, a, b, c, d, e ) (p)->lpVtbl->GetProcAmpRange( p, a, b, c, d, e )
#define IDirectXVideoProcessorService_GetFilterPropertyRange( p, a, b, c, d, e ) (p)->lpVtbl->GetFilterPropertyRange( p, a, b, c, d, e )
#define IDirectXVideoProcessorService_CreateVideoProcessor( p, a, b, c, d, e ) (p)->lpVtbl->CreateVideoProcessor( p, a, b, c, d, e )
#else
#define IDirectXVideoProcessorService_QueryInterface( p, a, b ) (p)->QueryInterface( a, b )
#define IDirectXVideoProcessorService_AddRef( p )  (p)->AddRef()
#define IDirectXVideoProcessorService_Release( p ) (p)->Release()
#define IDirectXVideoProcessorService_CreateSurface( p, a, b, c, d, e, f, g, h, i ) (p)->CreateSurface( a, b, c, d, e, f, g, h, i )
#define IDirectXVideoProcessorService_RegisterVideoProcessorSoftwareDevice( p, a ) (p)->RegisterVideoProcessorSoftwareDevice( a )
#define IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids( p, a, b, c ) (p)->GetVideoProcessorDeviceGuids( a, b, c )
#define IDirectXVideoProcessorService_GetVideoProcessorRenderTargets( p, a, b, c, d ) (p)->GetVideoProcessorRenderTargets( a, b, c, d )
#define IDirectXVideoProcessorService_GetVideoProcessorSubStreamFormats( p, a, b, c, d, e ) (p)->GetVideoProcessorSubStreamFormats( a, b, c, d, e )
#define IDirectXVideoProcessorService_GetVideoProcessorCaps( p, a, b, c, d ) (p)->GetVideoProcessorCaps( a, b, c, d )
#define IDirectXVideoProcessorService_GetProcAmpRange( p, a, b, c, d, e ) (p)->GetProcAmpRange( a, b, c, d, e )
#define IDirectXVideoProcessorService_GetFilterPropertyRange( p, a, b, c, d, e ) (p)->GetFilterPropertyRange( a, b, c, d, e )
#define IDirectXVideoProcessorService_CreateVideoProcessor( p, a, b, c, d, e ) (p)->CreateVideoProcessor( a, b, c, d, e )
#endif


/*****************************************************************************************************
************************DXVA Video Processor********************************************************
*******************************************************************************************************/



/*#undef INTERFACE
#define INTERFACE IDirectXVideoService
DECLARE_INTERFACE_(IDirectXVideoService,IUnknown)
{
    STDMETHOD(DXVA2CreateVideoService)(IDirect3DDevice9*, REFIID, void**) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectXVideoService_DXVA2CreateVideoService(a,b,c) DXVA2CreateVideoService(a,b,c)
#else
#define IDirectXVideoService_DXVA2CreateVideoService(a,b,c) DXVA2CreateVideoService(a,b,c)
#endif*/


#ifdef __cplusplus
};
#endif

#ifdef __cplusplus
extern "C" HRESULT WINAPI DXVA2CreateVideoService( IDirect3DDevice9 *,
                                                   REFIID riid,
                                                   void **ppService );
#else
extern HRESULT WINAPI DXVA2CreateVideoService( IDirect3DDevice9 *,
                                               REFIID riid,
                                               void **ppService );
#endif

typedef
enum _DXVA2_VideoChromaSubSampling
{   DXVA2_VideoChromaSubsamplingMask    = 0xf,
    DXVA2_VideoChromaSubsampling_Unknown    = 0,
    DXVA2_VideoChromaSubsampling_ProgressiveChroma  = 0x8,
    DXVA2_VideoChromaSubsampling_Horizontally_Cosited   = 0x4,
    DXVA2_VideoChromaSubsampling_Vertically_Cosited = 0x2,
    DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes = 0x1,
    DXVA2_VideoChromaSubsampling_MPEG2  = ( DXVA2_VideoChromaSubsampling_Horizontally_Cosited | DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes ),
    DXVA2_VideoChromaSubsampling_MPEG1  = DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes,
    DXVA2_VideoChromaSubsampling_DV_PAL = ( DXVA2_VideoChromaSubsampling_Horizontally_Cosited | DXVA2_VideoChromaSubsampling_Vertically_Cosited ),
    DXVA2_VideoChromaSubsampling_Cosited    = ( ( DXVA2_VideoChromaSubsampling_Horizontally_Cosited | DXVA2_VideoChromaSubsampling_Vertically_Cosited )  | DXVA2_VideoChromaSubsampling_Vertically_AlignedChromaPlanes )}   DXVA2_VideoChromaSubSampling;

typedef
enum _DXVA2_NominalRange
{   DXVA2_NominalRangeMask  = 0x7,
    DXVA2_NominalRange_Unknown  = 0,
    DXVA2_NominalRange_Normal   = 1,
    DXVA2_NominalRange_Wide = 2,
    DXVA2_NominalRange_0_255    = 1,
    DXVA2_NominalRange_16_235   = 2,
    DXVA2_NominalRange_48_208   = 3}   DXVA2_NominalRange;

typedef
enum _DXVA2_VideoLighting
{   DXVA2_VideoLightingMask = 0xf,
    DXVA2_VideoLighting_Unknown = 0,
    DXVA2_VideoLighting_bright  = 1,
    DXVA2_VideoLighting_office  = 2,
    DXVA2_VideoLighting_dim = 3,
    DXVA2_VideoLighting_dark    = 4}   DXVA2_VideoLighting;

typedef
enum _DXVA2_VideoPrimaries
{   DXVA2_VideoPrimariesMask    = 0x1f,
    DXVA2_VideoPrimaries_Unknown    = 0,
    DXVA2_VideoPrimaries_reserved   = 1,
    DXVA2_VideoPrimaries_BT709  = 2,
    DXVA2_VideoPrimaries_BT470_2_SysM   = 3,
    DXVA2_VideoPrimaries_BT470_2_SysBG  = 4,
    DXVA2_VideoPrimaries_SMPTE170M  = 5,
    DXVA2_VideoPrimaries_SMPTE240M  = 6,
    DXVA2_VideoPrimaries_EBU3213    = 7,
    DXVA2_VideoPrimaries_SMPTE_C    = 8}   DXVA2_VideoPrimaries;

typedef
enum _DXVA2_VideoTransferFunction
{   DXVA2_VideoTransFuncMask    = 0x1f,
    DXVA2_VideoTransFunc_Unknown    = 0,
    DXVA2_VideoTransFunc_10 = 1,
    DXVA2_VideoTransFunc_18 = 2,
    DXVA2_VideoTransFunc_20 = 3,
    DXVA2_VideoTransFunc_22 = 4,
    DXVA2_VideoTransFunc_709    = 5,
    DXVA2_VideoTransFunc_240M   = 6,
    DXVA2_VideoTransFunc_sRGB   = 7,
    DXVA2_VideoTransFunc_28 = 8}   DXVA2_VideoTransferFunction;

typedef
enum _DXVA2_SampleFormat
{   DXVA2_SampleFormatMask  = 0xff,
    DXVA2_SampleUnknown = 0,
    DXVA2_SampleProgressiveFrame    = 2,
    DXVA2_SampleFieldInterleavedEvenFirst   = 3,
    DXVA2_SampleFieldInterleavedOddFirst    = 4,
    DXVA2_SampleFieldSingleEven = 5,
    DXVA2_SampleFieldSingleOdd  = 6,
    DXVA2_SampleSubStream   = 7}   DXVA2_SampleFormat;

typedef
enum _DXVA2_VideoTransferMatrix
{   DXVA2_VideoTransferMatrixMask   = 0x7,
    DXVA2_VideoTransferMatrix_Unknown   = 0,
    DXVA2_VideoTransferMatrix_BT709 = 1,
    DXVA2_VideoTransferMatrix_BT601 = 2,
    DXVA2_VideoTransferMatrix_SMPTE240M = 3}   DXVA2_VideoTransferMatrix;

enum __MIDL___MIDL_itf_dxva2api_0000_0000_0004
{   DXVA2_NoiseFilterLumaLevel  = 1,
    DXVA2_NoiseFilterLumaThreshold  = 2,
    DXVA2_NoiseFilterLumaRadius = 3,
    DXVA2_NoiseFilterChromaLevel    = 4,
    DXVA2_NoiseFilterChromaThreshold    = 5,
    DXVA2_NoiseFilterChromaRadius   = 6,
    DXVA2_DetailFilterLumaLevel = 7,
    DXVA2_DetailFilterLumaThreshold = 8,
    DXVA2_DetailFilterLumaRadius    = 9,
    DXVA2_DetailFilterChromaLevel   = 10,
    DXVA2_DetailFilterChromaThreshold   = 11,
    DXVA2_DetailFilterChromaRadius  = 12};

enum __MIDL___MIDL_itf_dxva2api_0000_0000_0008
{   DXVA2_VideoProcess_None = 0,
    DXVA2_VideoProcess_YUV2RGB  = 0x1,
    DXVA2_VideoProcess_StretchX = 0x2,
    DXVA2_VideoProcess_StretchY = 0x4,
    DXVA2_VideoProcess_AlphaBlend   = 0x8,
    DXVA2_VideoProcess_SubRects = 0x10,
    DXVA2_VideoProcess_SubStreams   = 0x20,
    DXVA2_VideoProcess_SubStreamsExtended   = 0x40,
    DXVA2_VideoProcess_YUV2RGBExtended  = 0x80,
    DXVA2_VideoProcess_AlphaBlendExtended   = 0x100,
    DXVA2_VideoProcess_Constriction = 0x200,
    DXVA2_VideoProcess_NoiseFilter  = 0x400,
    DXVA2_VideoProcess_DetailFilter = 0x800,
    DXVA2_VideoProcess_PlanarAlpha  = 0x1000,
    DXVA2_VideoProcess_LinearScaling    = 0x2000,
    DXVA2_VideoProcess_GammaCompensated = 0x4000,
    DXVA2_VideoProcess_MaintainsOriginalFieldData   = 0x8000,
    DXVA2_VideoProcess_Mask = 0xffff};



__inline float hb_dx_fixedtofloat( const DXVA2_Fixed32 _fixed_ );

__inline const DXVA2_Fixed32 hb_dx_fixed32_opaque_alpha();

__inline DXVA2_Fixed32 hb_dx_floattofixed( const float _float_ );
#endif
#endif //_DXVA2API_H
