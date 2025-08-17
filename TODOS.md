# Todos

## Timing Drift with Large Buffer Sizes

**Issue**: Timing drift occurs when using buffer sizes of 1024 samples and above.

**Description**: The plugin's timing precision degrades significantly with larger buffer sizes, causing the generated beats to drift from the host's tempo grid.

**Affected Buffer Sizes**: 1024 samples and above

**Workaround**: Use smaller buffer sizes (512 samples or less) for optimal timing precision.

**Status**: Known issue, investigation needed

## Complete Lack of State Persistence

**Issue**: The plugin has no state persistence - all settings, samples, and patterns are lost when the DAW project is reopened.

**Description**: The plugin does not implement any state saving/loading functionality. When DAWs like Ableton save and reopen a project, the following are completely lost:
- Loaded audio sample (reverts to default click sample)
- ADSR envelope settings (Attack, Decay, Sustain, Release)
- Current step sequencer pattern (which steps are active/inactive)

**Affected Elements**: 
- Audio samples
- ADSR parameters (Attack, Decay, Sustain, Release) 
- Step sequencer pattern state

**Workaround**: Manually reload samples, adjust ADSR settings, and recreate step patterns each time the plugin is reopened.

**Status**: Known issue, investigation needed

## Sample Playback Inconsistency with High Release Values

**Issue**: Setting the release time high on long samples causes inconsistent sample playback behavior.

**Description**: When using long audio samples with high release values, the sample playback becomes unpredictable and inconsistent. This appears to be related to how the ADSR envelope and sample length interact.

**Affected Scenarios**: Long audio samples with high release time settings

**Workaround**: Use shorter release times or shorter samples to maintain consistent playback.

**Status**: Known issue, investigation needed

## ADSR Slider UI Issues

**Issue**: The ADSR sliders are ugly and unintuitive to use.

**Description**: The current slider implementation has poor visual design and user experience, making it difficult for users to understand and adjust the ADSR envelope parameters effectively.

**Affected Elements**: Attack, Decay, Sustain, and Release sliders

**Workaround**: None currently available

**Status**: Known issue, investigation needed

## ADSR Parameters Cannot Be Automated by Host

**Issue**: ADSR envelope parameters (Attack, Decay, Sustain, Release) are not exposed as plugin parameters to the host DAW.

**Description**: The plugin does not implement the necessary parameter system to expose ADSR controls to the host (like Ableton). This means the parameters cannot be automated, recorded, or controlled by the host's automation system.

**Affected Parameters**: Attack, Decay, Sustain, Release

**Workaround**: None currently available - parameters must be adjusted manually within the plugin

**Status**: Known issue, investigation needed

## Extendable Pattern Length

**Issue**: Pattern length is fixed at 16 steps and cannot be extended or shortened.

**Description**: The step sequencer is currently limited to exactly 16 steps. Users cannot extend the pattern to 32, 48, or 64 steps for longer musical phrases, or shorten it for shorter patterns.

**Affected Elements**: Step sequencer pattern length

**Proposed Solution**: Add +/- buttons to allow extending the pattern by additional 16-step rows or removing rows to create shorter patterns.

**Status**: Feature request, investigation needed

## Visual Step Indicator

**Issue**: No visual indication of which step is currently playing in the sequencer.

**Description**: The step sequencer buttons don't show which step is currently active during playback, making it difficult for users to follow along with the pattern or identify timing issues.

**Affected Elements**: Step sequencer button visual feedback

**Proposed Solution**: Highlight or animate the currently playing step button to show real-time playback position.

**Status**: ✅ **COMPLETED** - Implemented with real-time step highlighting

**Implementation Details**:
- Added Timer functionality to PluginEditor (30Hz updates)
- Current playing step is highlighted in orange
- Active steps remain green, inactive steps remain dark grey
- Visual indicator updates in real-time during playback
- Preserves step state when toggling buttons during playback
- Uses `getLastTriggeredStep()` from processor for accurate timing