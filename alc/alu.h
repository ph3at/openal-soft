#ifndef ALU_H
#define ALU_H

#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

#include "alspan.h"
#include "core/ambidefs.h"
#include "core/bufferline.h"
#include "core/devformat.h"

struct ALCcontext;
struct ALCdevice;
struct ALeffectslot;
struct MixParams;


#define MAX_SENDS  6


using MixerFunc = void(*)(const al::span<const float> InSamples,
    const al::span<FloatBufferLine> OutBuffer, float *CurrentGains, const float *TargetGains,
    const size_t Counter, const size_t OutPos);

extern MixerFunc MixSamples;


constexpr float GainMixMax{1000.0f}; /* +60dB */

constexpr float SpeedOfSoundMetersPerSec{343.3f};
constexpr float AirAbsorbGainHF{0.99426f}; /* -0.05dB */

/** Target gain for the reverb decay feedback reaching the decay time. */
constexpr float ReverbDecayGain{0.001f}; /* -60 dB */


enum HrtfRequestMode {
    Hrtf_Default = 0,
    Hrtf_Enable = 1,
    Hrtf_Disable = 2,
};

void aluInit(void);

void aluInitMixer(void);

/* aluInitRenderer
 *
 * Set up the appropriate panning method and mixing method given the device
 * properties.
 */
void aluInitRenderer(ALCdevice *device, int hrtf_id, HrtfRequestMode hrtf_appreq,
    HrtfRequestMode hrtf_userreq);

void aluInitEffectPanning(ALeffectslot *slot, ALCcontext *context);

/**
 * Calculates ambisonic encoder coefficients using the X, Y, and Z direction
 * components, which must represent a normalized (unit length) vector, and the
 * spread is the angular width of the sound (0...tau).
 *
 * NOTE: The components use ambisonic coordinates. As a result:
 *
 * Ambisonic Y = OpenAL -X
 * Ambisonic Z = OpenAL Y
 * Ambisonic X = OpenAL -Z
 *
 * The components are ordered such that OpenAL's X, Y, and Z are the first,
 * second, and third parameters respectively -- simply negate X and Z.
 */
std::array<float,MaxAmbiChannels> CalcAmbiCoeffs(const float y, const float z, const float x,
    const float spread);

/**
 * CalcDirectionCoeffs
 *
 * Calculates ambisonic coefficients based on an OpenAL direction vector. The
 * vector must be normalized (unit length), and the spread is the angular width
 * of the sound (0...tau).
 */
inline std::array<float,MaxAmbiChannels> CalcDirectionCoeffs(const float (&dir)[3],
    const float spread)
{
    /* Convert from OpenAL coords to Ambisonics. */
    return CalcAmbiCoeffs(-dir[0], dir[1], -dir[2], spread);
}

/**
 * CalcAngleCoeffs
 *
 * Calculates ambisonic coefficients based on azimuth and elevation. The
 * azimuth and elevation parameters are in radians, going right and up
 * respectively.
 */
inline std::array<float,MaxAmbiChannels> CalcAngleCoeffs(const float azimuth,
    const float elevation, const float spread)
{
    const float x{-std::sin(azimuth) * std::cos(elevation)};
    const float y{ std::sin(elevation)};
    const float z{ std::cos(azimuth) * std::cos(elevation)};

    return CalcAmbiCoeffs(x, y, z, spread);
}


/**
 * ComputePanGains
 *
 * Computes panning gains using the given channel decoder coefficients and the
 * pre-calculated direction or angle coefficients. For B-Format sources, the
 * coeffs are a 'slice' of a transform matrix for the input channel, used to
 * scale and orient the sound samples.
 */
void ComputePanGains(const MixParams *mix, const float*RESTRICT coeffs, const float ingain,
    const al::span<float,MAX_OUTPUT_CHANNELS> gains);


/** Helper to set an identity/pass-through panning for ambisonic mixing (3D input). */
template<typename T, typename I, typename F>
auto SetAmbiPanIdentity(T iter, I count, F func) -> std::enable_if_t<std::is_integral<I>::value>
{
    if(count < 1) return;

    std::array<float,MaxAmbiChannels> coeffs{{1.0f}};
    func(*iter, coeffs);
    ++iter;
    for(I i{1};i < count;++i,++iter)
    {
        coeffs[i-1] = 0.0f;
        coeffs[i  ] = 1.0f;
        func(*iter, coeffs);
    }
}


extern const float ConeScale;
extern const float ZScale;

#endif
