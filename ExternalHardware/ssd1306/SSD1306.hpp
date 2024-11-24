#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <AbstractPlatform/common/Platform.hpp>
#include <AbstractPlatform/common/ErrorCode.hpp>
#include <AbstractPlatform/common/PlatformLiteral.hpp>
#include <AbstractPlatform/i2c/AbstractI2C.hpp>

namespace ExternalHardware
{

class CSsd1306
{
public:
    static constexpr std::uint8_t KDefaultAddress = 0x3C;      // SA0 pulled to GND
    static constexpr std::uint8_t KAlternativeAddress = 0x3D;  // SA0 pulled to VS

    // Fundamental Command Table
    static constexpr std::uint8_t KCmdDisplayOn = 0xAF;  // Default
    static constexpr std::uint8_t KCmdDisplayOff = 0xAE;
    static constexpr std::uint8_t KCmdEnableRamDisplay = 0xA4;  // Default
    static constexpr std::uint8_t KCmdDisableRamDisplay = 0xA5;
    static constexpr std::uint8_t KCmdNormalDisplay = 0xA6;  // Default
    static constexpr std::uint8_t KCmdInverseDisplay = 0xA7;
    static constexpr std::uint8_t KCmdContrast = 0x81;

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
        CConfig config;
        return SetConfig( config ) == AbstractPlatform::KOk;
    }

    inline AbstractPlatform::TErrorCode
    Reset( CConfig aConfig ) NOEXCEPT
    {
        auto result = SetConfig( aConfig ) == AbstractPlatform::KOk;
        if ( result == AbstractPlatform::KOk )
        {
            return SetConfig( aConfig );
        }
        return result;
    }

    AbstractPlatform::TErrorCode GetConfig( CConfig& aConfig ) NOEXCEPT;
    AbstractPlatform::TErrorCode SetConfig( const CConfig& aConfig ) NOEXCEPT;

    inline AbstractPlatform::TErrorCode
    EnableRamDisplay( bool aEnable ) NOEXCEPT
    {
        return SendCommand( aEnable ? KCmdEnableRamDisplay : KCmdDisableRamDisplay );
    }

    inline AbstractPlatform::TErrorCode
    DisplayEnable( bool aOn ) NOEXCEPT
    {
        // The KCmdDisplayOff switches the display to the sleep mode
        return SendCommand( aOn ? KCmdDisplayOn : KCmdDisplayOff );
    }

    inline AbstractPlatform::TErrorCode
    InverseDisplay( bool aInverse = false ) NOEXCEPT
    {
        return SendCommand( aInverse ? KCmdInverseDisplay : KCmdNormalDisplay );
    }

    inline AbstractPlatform::TErrorCode
    SetContrast( std::uint8_t aContrast = 0x7f ) NOEXCEPT
    {
        const std::uint8_t commands[] = { KCmdContrast, aContrast };
        return SendCommands( commands, sizeof( commands ) );
    }

#ifdef __EXCEPTIONS

#endif

    AbstractPlatform::TErrorCode SendCommand( uint8_t aCommand, bool aNoStop = false ) NOEXCEPT;
    AbstractPlatform::TErrorCode SendCommands( const uint8_t* aCommands,
                                               size_t aCommandsNumber ) NOEXCEPT;

private:
    template < typename taRegisterType >
    AbstractPlatform::TErrorCode ReadRegister( std::uint8_t aReg,
                                               taRegisterType& aRegisterValue ) NOEXCEPT;
    template < typename taRegisterType >
    AbstractPlatform::TErrorCode WriteRegister( std::uint8_t aReg,
                                                taRegisterType aRegisterValue ) NOEXCEPT;

    AbstractPlatform::TErrorCode SendBuffer( uint8_t aBuffer[],
                                             size_t aBufferSize,
                                             bool aAutoReleaseMemory = false ) NOEXCEPT;

    /* data */
    AbstractPlatform::IAbstractI2CBus& iI2CBus;
    const std::uint8_t iDeviceAddress;
    std::vector< std::uint8_t > iDataBuffer;
};

}  // namespace ExternalHardware