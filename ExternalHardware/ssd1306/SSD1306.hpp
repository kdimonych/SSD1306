#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>
#include <AbstractPlatform/common/Platform.hpp>
#include <AbstractPlatform/common/ErrorCode.hpp>
#include <AbstractPlatform/common/ArrayHelper.hpp>
#include <AbstractPlatform/i2c/AbstractI2C.hpp>

namespace ExternalHardware
{

class CSsd1306
{
public:
    using TErrorCode = AbstractPlatform::TErrorCode;

    static constexpr std::uint8_t KDefaultAddress = 0x3C;      // SA0 pulled to GND
    static constexpr std::uint8_t KAlternativeAddress = 0x3D;  // SA0 pulled to VS

    CSsd1306( AbstractPlatform::IAbstractI2CBus& aI2CBus,
              std::uint8_t aDeviceAddress = KDefaultAddress ) NOEXCEPT;
    ~CSsd1306( ) = default;

    // Fundamental Command
    inline TErrorCode
    EnableRamDisplay( bool aEnable ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdEnableRamDisplay = 0xA4;  // Default
        constexpr std::uint8_t KCmdDisableRamDisplay = 0xA5;

        return SendCommand( aEnable ? KCmdEnableRamDisplay : KCmdDisableRamDisplay );
    }

    inline TErrorCode
    DisplayEnable( bool aOn ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdDisplayOn = 0xAF;   // Default
        constexpr std::uint8_t KCmdDisplayOff = 0xAE;  // Switches the display to the sleep mode

        return SendCommand( aOn ? KCmdDisplayOn : KCmdDisplayOff );
    }

    inline TErrorCode
    InverseDisplay( bool aInverse = false ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdNormalDisplay = 0xA6;  // Default
        constexpr std::uint8_t KCmdInverseDisplay = 0xA7;

        return SendCommand( aInverse ? KCmdInverseDisplay : KCmdNormalDisplay );
    }

    inline TErrorCode
    SetContrast( std::uint8_t aContrast = 0x7f ) NOEXCEPT
    {
        using namespace AbstractPlatform;

        constexpr std::uint8_t KCmdContrast = 0x81;

        const std::uint8_t commands[] = { KCmdContrast, aContrast };
        return SendCommands( commands, ArrayLength( commands ) );
    }

    // Scrolling Command
    enum ScrollStepInterval
    {
        Step2Frame = 0x07,
        Step3Frame = 0x04,
        Step4Frame = 0x05,
        Step5Frame = 0x00,
        Step25Frame = 0x06,
        Step64Frame = 0x01,
        Step128Frames = 0x02,
        Step254Frames = 0x03
    };

    TErrorCode
    ContinuousHorizontalScroll( bool aScrollDirectionLeft,
                                std::uint8_t aStartPage,
                                std::uint8_t aEndPage,
                                ScrollStepInterval aScrollStepInterval ) NOEXCEPT
    {
        using namespace AbstractPlatform;

        constexpr std::uint8_t KCommand = 0x26;
        constexpr std::uint8_t KDummyByte = 0x00;
        constexpr std::uint8_t KDummyEndByte = 0xff;

        assert( aStartPage & 0x07 == aStartPage );
        assert( aEndPage & 0x07 == aEndPage );
        assert( aStartPage <= aEndPage );
        assert( static_cast< std::uint8_t >( aScrollStepInterval )
                & 0x07 == static_cast< std::uint8_t >( aScrollStepInterval ) );

        const std::uint8_t commands[]
            = { static_cast< std::uint8_t >( KCommand | aScrollDirectionLeft ),
                KDummyByte,
                static_cast< std::uint8_t >( aStartPage & 0x07 ),
                static_cast< std::uint8_t >( aScrollStepInterval & 0x07 ),
                static_cast< std::uint8_t >( aEndPage & 0x07 ),
                KDummyByte,
                KDummyEndByte };
        return SendCommands( commands, ArrayLength( commands ) );
    }

    TErrorCode
    ContinuousVerticalAndHorizontalScroll( bool aScrollDirectionLeft,
                                           std::uint8_t aStartPage,
                                           std::uint8_t aEndPage,
                                           ScrollStepInterval aScrollStepInterval,
                                           std::uint8_t aVerticalScrollOffset ) NOEXCEPT
    {
        using namespace AbstractPlatform;

        constexpr std::uint8_t KScrollDirectionLeft = 0x29;
        constexpr std::uint8_t KScrollDirectionRight = 0x2A;
        constexpr std::uint8_t KDummyByte = 0x00;

        assert( aStartPage & 0x07 == aStartPage );
        assert( aEndPage & 0x07 == aEndPage );
        assert( aStartPage <= aEndPage );
        assert( static_cast< std::uint8_t >( aScrollStepInterval )
                & 0x07 == static_cast< std::uint8_t >( aScrollStepInterval ) );
        assert( aVerticalScrollOffset & 0x3F == aVerticalScrollOffset );

        const std::uint8_t commands[]
            = { aScrollDirectionLeft ? KScrollDirectionLeft : KScrollDirectionRight,
                KDummyByte,
                static_cast< std::uint8_t >( aStartPage & 0x07 ),
                static_cast< std::uint8_t >( aScrollStepInterval & 0x07 ),
                static_cast< std::uint8_t >( aEndPage & 0x07 ),
                static_cast< std::uint8_t >( aVerticalScrollOffset & 0x3F ) };
        return SendCommands( commands, ArrayLength( commands ) );
    }

    inline TErrorCode
    DeactivateScroll( ) NOEXCEPT
    {
        constexpr std::uint8_t KDeactivateScroll = 0x2E;
        return SendCommand( KDeactivateScroll );
    }

    inline TErrorCode
    ActivateScroll( ) NOEXCEPT
    {
        constexpr std::uint8_t KActivateScroll = 0x2F;
        return SendCommand( KActivateScroll );
    }

    TErrorCode
    SetVerticalScrollArea( std::uint8_t aNumberOfRowsInTopFixedArea,
                           std::uint8_t aNumberOfRowsInScrollArea ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KSetVerticalScrollArea = 0xA3;

        assert( aNumberOfRowsInTopFixedArea & 0x3F == aNumberOfRowsInTopFixedArea );
        assert( aNumberOfRowsInScrollArea & 0x7F == aNumberOfRowsInScrollArea );

        const std::uint8_t commands[] = {
            KSetVerticalScrollArea,
            static_cast< std::uint8_t >( aNumberOfRowsInTopFixedArea & 0x3F ),
            static_cast< std::uint8_t >( aNumberOfRowsInScrollArea & 0x7F ),
        };

        return SendCommands( commands, ArrayLength( commands ) );
    }

#ifdef __EXCEPTIONS

#endif

    TErrorCode SendCommand( uint8_t aCommand, bool aNoStop = false ) NOEXCEPT;
    TErrorCode SendCommands( const uint8_t* aCommands, size_t aCommandsNumber ) NOEXCEPT;

private:
    TErrorCode SendBuffer( uint8_t aBuffer[],
                           size_t aBufferSize,
                           bool aAutoReleaseMemory = false ) NOEXCEPT;

    /* data */
    AbstractPlatform::IAbstractI2CBus& iI2CBus;
    const std::uint8_t iDeviceAddress;
    std::vector< std::uint8_t > iDataBuffer;
};

}  // namespace ExternalHardware