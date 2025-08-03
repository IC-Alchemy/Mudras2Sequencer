#include "VoiceManager.h"
#include <algorithm>
#include <cstring>

VoiceManager::VoiceManager(uint8_t maxVoices) 
    : maxVoiceCount(maxVoices), nextVoiceId(1), sampleRate(48000.0f), globalVolume(1.0f) {
    voices.reserve(maxVoiceCount);
}

uint8_t VoiceManager::addVoice(const VoiceConfig& config) {
    if (!hasAvailableSlots()) {
        return 0; // No available slots
    }
    
    uint8_t voiceId = generateVoiceId();
    auto voice = std::make_unique<Voice>(voiceId, config);
    voice->init(sampleRate);
    
    auto managedVoice = std::make_unique<ManagedVoice>(std::move(voice), voiceId);
    voices.push_back(std::move(managedVoice));
    
    notifyVoiceCountChanged();
    return voiceId;
}

uint8_t VoiceManager::addVoice(const std::string& presetName) {
    VoiceConfig config = getPresetConfig(presetName);
    return addVoice(config);
}

bool VoiceManager::removeVoice(uint8_t voiceId) {
    auto it = std::find_if(voices.begin(), voices.end(),
        [voiceId](const std::unique_ptr<ManagedVoice>& v) {
            return v->id == voiceId;
        });
    
    if (it != voices.end()) {
        voices.erase(it);
        notifyVoiceCountChanged();
        return true;
    }
    
    return false;
}

void VoiceManager::removeAllVoices() {
    voices.clear();
    notifyVoiceCountChanged();
}

bool VoiceManager::setVoiceConfig(uint8_t voiceId, const VoiceConfig& config) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        managedVoice->voice->setConfig(config);
        return true;
    }
    return false;
}

bool VoiceManager::setVoicePreset(uint8_t voiceId, const std::string& presetName) {
    VoiceConfig config = getPresetConfig(presetName);
    return setVoiceConfig(voiceId, config);
}

VoiceConfig* VoiceManager::getVoiceConfig(uint8_t voiceId) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        return &managedVoice->voice->getConfig();
    }
    return nullptr;
}

bool VoiceManager::updateVoiceState(uint8_t voiceId, const VoiceState& state) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        managedVoice->voice->updateParameters(state);
        notifyVoiceUpdated(voiceId, state);
        return true;
    }
    return false;
}

VoiceState* VoiceManager::getVoiceState(uint8_t voiceId) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        return &managedVoice->voice->getState();
    }
    return nullptr;
}

bool VoiceManager::attachSequencer(uint8_t voiceId, std::unique_ptr<Sequencer> sequencer) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        managedVoice->voice->setSequencer(std::move(sequencer));
        return true;
    }
    return false;
}

bool VoiceManager::attachSequencer(uint8_t voiceId, Sequencer* sequencer) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice && sequencer) {
        // For raw pointers, we don't transfer ownership
        // The Voice class needs to handle this case
        managedVoice->voice->setSequencer(sequencer);
        return true;
    }
    return false;
}

Sequencer* VoiceManager::getSequencer(uint8_t voiceId) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        return managedVoice->voice->getSequencer();
    }
    return nullptr;
}

void VoiceManager::init(float sr) {
    sampleRate = sr;
    for (auto& managedVoice : voices) {
        if (managedVoice->voice) {
            managedVoice->voice->init(sampleRate);
        }
    }
}

float VoiceManager::processAllVoices() {
    float mixedOutput = 0.0f;
    
    for (auto& managedVoice : voices) {
        if (managedVoice->enabled && managedVoice->voice) {
            float voiceOutput = managedVoice->voice->process();
            mixedOutput += voiceOutput * managedVoice->mixLevel;
        }
    }
    
    return mixedOutput * globalVolume;
}

float VoiceManager::processVoice(uint8_t voiceId) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->enabled && managedVoice->voice) {
        return managedVoice->voice->process() * managedVoice->mixLevel * globalVolume;
    }
    return 0.0f;
}

void VoiceManager::enableVoice(uint8_t voiceId, bool enabled) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice) {
        managedVoice->enabled = enabled;
    }
}

void VoiceManager::disableVoice(uint8_t voiceId) {
    enableVoice(voiceId, false);
}

bool VoiceManager::isVoiceEnabled(uint8_t voiceId) const {
    const ManagedVoice* managedVoice = findVoice(voiceId);
    return managedVoice ? managedVoice->enabled : false;
}

std::vector<uint8_t> VoiceManager::getActiveVoiceIds() const {
    // OPTIMIZATION: Pre-allocate with exact size to avoid dynamic reallocation
    std::vector<uint8_t> activeIds;
    activeIds.reserve(voices.size()); // Reserve maximum possible size
    
    for (const auto& managedVoice : voices) {
        if (managedVoice->enabled) {
            activeIds.push_back(managedVoice->id);
        }
    }
    
    return activeIds;
}

size_t VoiceManager::getMemoryUsage() const {
    size_t totalSize = sizeof(VoiceManager);
    
    // Add size of voice vector and managed voices
    totalSize += voices.capacity() * sizeof(std::unique_ptr<ManagedVoice>);
    
    for (const auto& managedVoice : voices) {
        totalSize += sizeof(ManagedVoice);
        if (managedVoice->voice) {
            totalSize += sizeof(Voice);
            // Add approximate size of voice components
            totalSize += managedVoice->voice->getConfig().oscillatorCount * sizeof(daisysp::Oscillator);
            totalSize += sizeof(daisysp::LadderFilter); // Filter
            totalSize += sizeof(daisysp::Svf);        // High-pass filter
            totalSize += sizeof(daisysp::Adsr);       // Envelope
        }
    }
    
    return totalSize;
}

void VoiceManager::setVoiceMix(uint8_t voiceId, float mix) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice) {
        managedVoice->mixLevel = std::max(0.0f, std::min(1.0f, mix));
    }
}

float VoiceManager::getVoiceMix(uint8_t voiceId) const {
    const ManagedVoice* managedVoice = findVoice(voiceId);
    return managedVoice ? managedVoice->mixLevel : 0.0f;
}

void VoiceManager::setVoiceOutput(uint8_t voiceId, uint8_t outputChannel) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice) {
        managedVoice->outputChannel = outputChannel;
    }
}

uint8_t VoiceManager::getVoiceOutput(uint8_t voiceId) const {
    const ManagedVoice* managedVoice = findVoice(voiceId);
    return managedVoice ? managedVoice->outputChannel : 0;
}

// Static methods for preset management
std::vector<std::string> VoiceManager::getAvailablePresets() {
    return {
        "analog",
        "digital", 
        "bass",
        "lead",
        "pad",
        "percussion"
    };
}

VoiceConfig VoiceManager::getPresetConfig(const std::string& presetName) {
    if (presetName == "analog") {
        return VoicePresets::getAnalogVoice();
    } else if (presetName == "digital") {
        return VoicePresets::getDigitalVoice();
    } else if (presetName == "bass") {
        return VoicePresets::getBassVoice();
    } else if (presetName == "lead") {
        return VoicePresets::getLeadVoice();
    } else if (presetName == "pad") {
        return VoicePresets::getPadVoice();
    } else if (presetName == "percussion") {
        return VoicePresets::getPercussionVoice();
    } else {
        // Default to analog voice if preset not found
        return VoicePresets::getAnalogVoice();
    }
}

// Private helper methods - OPTIMIZED for embedded performance
VoiceManager::ManagedVoice* VoiceManager::findVoice(uint8_t voiceId) {
    // OPTIMIZATION: Use direct iteration instead of std::find_if for better embedded performance
    for (auto& voice : voices) {
        if (voice->id == voiceId) {
            return voice.get();
        }
    }
    return nullptr;
}

const VoiceManager::ManagedVoice* VoiceManager::findVoice(uint8_t voiceId) const {
    // OPTIMIZATION: Use direct iteration instead of std::find_if for better embedded performance
    for (const auto& voice : voices) {
        if (voice->id == voiceId) {
            return voice.get();
        }
    }
    return nullptr;
}

uint8_t VoiceManager::generateVoiceId() {
    uint8_t id = nextVoiceId;
    nextVoiceId++;
    
    // Handle overflow and ensure unique IDs
    if (nextVoiceId == 0) {
        nextVoiceId = 1;
    }
    
    // Ensure the ID is unique (in case of overflow)
    while (findVoice(id) != nullptr) {
        id = nextVoiceId++;
        if (nextVoiceId == 0) {
            nextVoiceId = 1;
        }
    }
    
    return id;
}

void VoiceManager::notifyVoiceCountChanged() {
    if (voiceCountCallback) {
        voiceCountCallback(getVoiceCount());
    }
}

void VoiceManager::notifyVoiceUpdated(uint8_t voiceId, const VoiceState& state) {
    if (voiceUpdateCallback) {
        voiceUpdateCallback(voiceId, state);
    }
}

// Voice Parameter Control Methods
void VoiceManager::setVoiceVolume(uint8_t voiceId, float volume) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        managedVoice->mixLevel = std::max(0.0f, std::min(1.0f, volume));
    }
}

void VoiceManager::setVoiceFrequency(uint8_t voiceId, float frequency) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        managedVoice->voice->setFrequency(frequency);
    }
}

void VoiceManager::setVoiceSlide(uint8_t voiceId, float slideTime) {
    ManagedVoice* managedVoice = findVoice(voiceId);
    if (managedVoice && managedVoice->voice) {
        managedVoice->voice->setSlideTime(slideTime);
    }
}