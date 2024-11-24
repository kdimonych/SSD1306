#pragma once

#include <cstdint>
#include <cstring>
#include <AbstractPlatform/common/Platform.hpp>
#include <AbstractPlatform/common/ErrorCode.hpp>
#include <AbstractPlatform/common/PlatformLiteral.hpp>
#include <AbstractPlatform/i2c/AbstractI2C.hpp>

namespace ExternalHardware
{

class CSsd1306
{
public:
    static constexpr std::uint8_t KDefaultAddress = 0x40;  // A0 pulled to GND
    static constexpr std::uint8_t KVSAddress = 0x41;       // A0 pulled to VS

    CSsd1306( AbstractPlatform::IAbstractI2CBus& aI2CBus,
              std::uint8_t aDeviceAddress = KDefaultAddress ) NOEXCEPT;
    ~CSsd1306( ) = default;
    struct CConfig
    {
        CConfig( ){ };
    };

    int Init( const CConfig& aConfig = { } ) NOEXCEPT;

    inline AbstractPlatform::TErrorCode
    Reset( ) NOEXCEPT
    {
        // TODO: implement
        return AbstractPlatform::KOk;
    }

    inline AbstractPlatform::TErrorCode
    Reset( CConfig aConfig ) NOEXCEPT
    {
        // TODO: implement
        return AbstractPlatform::KOk;
    }

    AbstractPlatform::TErrorCode GetConfig( CConfig& aConfig ) NOEXCEPT;
    AbstractPlatform::TErrorCode SetConfig( const CConfig& aConfig ) NOEXCEPT;

#ifdef __EXCEPTIONS

#endif

private:
    /* data */
    AbstractPlatform::IAbstractI2CBus& iI2CBus;
    const std::uint8_t iDeviceAddress;
};

}  // namespace ExternalHardware