#include "config_loader.hpp"

#include "config_schema.hpp"

namespace bandwagon::kernel_expansion {

bool LoadPeerPressureConfig(ConfigurationState* state,
                            ConfigBootstrapServices* bootstrap,
                            FileServices* files,
                            StructuredConfigParser* parser,
                            ConfigServices* config_services) {
  if (!state || !bootstrap || !files || !parser || !config_services)
    return false;

  bootstrap->InitializeStateLock();

  const ConfigAllocationRequest request{};
  const int allocation_status =
      bootstrap->AllocateConfig(request, &state->singleton);
  if (allocation_status != 0 || !state->singleton)
    bootstrap->DisplayFatalError(kConfigFatalErrorCode);

  // VdDisplayFatalError is effectively non-returning on the target. Keep the
  // host lift safe when a test double returns from that call.
  if (!state->singleton) return false;

  std::uint8_t* probe = nullptr;
  std::uint32_t probe_size = 0;
  bool result = false;
  if (!ReadFileToAllocatedBuffer(files, kPeerPressureConfigPath, &probe,
                                 &probe_size)) {
    result = InitializeDefaultConfig(state->singleton, config_services);
  } else {
    result = LoadStructuredConfigFile(
        files, parser, kPeerPressureConfigPath,
        {state->singleton->Data(), state->singleton->Size()},
        kPeerPressureConfigSchema.data(), kPeerPressureConfigSchema.size());
    if (!result) {
      *state->singleton = PeerPressureConfig{};
      state->singleton->check_for_updates_on_startup = 1;
    }
  }

  if (probe) files->Free(probe);
  return result;
}

}  // namespace bandwagon::kernel_expansion
