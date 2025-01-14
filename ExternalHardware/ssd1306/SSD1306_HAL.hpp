#pragma once

#include <AbstractPlatform/common/Platform.hpp>
#include <AbstractPlatform/common/ErrorCode.hpp>
#include <AbstractPlatform/common/ArrayHelper.hpp>
#include <AbstractPlatform/i2c/AbstractI2C.hpp>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>

namespace ExternalHardware
{
namespace Ssd1306
{

struct Ssd1306128x32
{
    using TPage = std::uint8_t;
    static constexpr std::uint8_t KPixelWidth = 128;
    static constexpr std::uint8_t KPixelHight = 32;
    static constexpr std::uint8_t KPixelsPerPage = 8;
};

struct Ssd1306128x64
{
    using TPage = std::uint8_t;
    static constexpr std::uint8_t KPixelWidth = 128;
    static constexpr std::uint8_t KPixelHight = 64;
    static constexpr std::uint8_t KPixelsPerPage = 8;
};

template < typename taDisplayType = Ssd1306128x32 >
class CSsd1306HalBase
{
public:
    using TErrorCode = AbstractPlatform::TErrorCode;
    using TPage = typename taDisplayType::TPage;

    static constexpr std::uint8_t KDefaultAddress = 0x3C;      // SA0 pulled to GND
    static constexpr std::uint8_t KAlternativeAddress = 0x3D;  // SA0 pulled to VS
    static constexpr std::uint8_t KPixelWidth = taDisplayType::KPixelWidth;
    static constexpr std::uint8_t KPixelHight = taDisplayType::KPixelHight;
    static constexpr std::uint8_t KPixelsPerPage = taDisplayType::KPixelsPerPage;
    static constexpr std::uint8_t KMaxColumns = taDisplayType::KPixelWidth;
    static constexpr std::uint8_t KMaxPages
        = static_cast< std::uint8_t >( ( taDisplayType::KPixelHight + 1 ) / KPixelsPerPage );
    static constexpr size_t KRamSize = KMaxColumns * KMaxPages * KPixelsPerPage / 8;
    static constexpr std::uint8_t KCmdSetRamBuffer = 0x40;

    CSsd1306HalBase( AbstractPlatform::IAbstractI2CBus& aI2CBus,
                     std::uint8_t aDeviceAddress = KDefaultAddress ) NOEXCEPT
        : iI2CBus{ aI2CBus },
          iDeviceAddress{ aDeviceAddress }
    {
    }

    ~CSsd1306HalBase( ) = default;

    // Fundamental Command
    inline TErrorCode
    EnableFillWholeRamWith( bool aBitValue ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdSetAllOn = 0xA5;
        constexpr std::uint8_t KCmdSetAllOff = 0xA4;

        return SendCommand( aBitValue ? KCmdSetAllOn : KCmdSetAllOff );
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
        return SendCommands( commands );
    }

    // Scrolling Command
    enum class TScrollStepInterval
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
                                TScrollStepInterval aScrollStepInterval ) NOEXCEPT
    {
        using namespace AbstractPlatform;

        constexpr std::uint8_t KCommand = 0x26;
        constexpr std::uint8_t KDummyByte = 0x00;
        constexpr std::uint8_t KDummyEndByte = 0xff;

        assert( aStartPage < KMaxPages );
        assert( aEndPage < KMaxPages );
        assert( aStartPage <= aEndPage );

        const std::uint8_t commands[]
            = { static_cast< std::uint8_t >( KCommand | aScrollDirectionLeft ),
                KDummyByte,
                static_cast< std::uint8_t >( aStartPage & 0x07 ),
                static_cast< std::uint8_t >( aScrollStepInterval & 0x07 ),
                static_cast< std::uint8_t >( aEndPage & 0x07 ),
                KDummyByte,
                KDummyEndByte };
        return SendCommands( commands );
    }

    TErrorCode
    ContinuousVerticalAndHorizontalScroll( bool aScrollDirectionLeft,
                                           std::uint8_t aStartPage,
                                           std::uint8_t aEndPage,
                                           TScrollStepInterval aScrollStepInterval,
                                           std::uint8_t aVerticalScrollOffset ) NOEXCEPT
    {
        using namespace AbstractPlatform;

        constexpr std::uint8_t KScrollDirectionLeft = 0x29;
        constexpr std::uint8_t KScrollDirectionRight = 0x2A;
        constexpr std::uint8_t KDummyByte = 0x00;

        assert( aStartPage < KMaxPages );
        assert( aEndPage < KMaxPages );
        assert( aStartPage <= aEndPage );
        assert( aVerticalScrollOffset <= 0x3F );

        const std::uint8_t commands[]
            = { aScrollDirectionLeft ? KScrollDirectionLeft : KScrollDirectionRight,
                KDummyByte,
                static_cast< std::uint8_t >( aStartPage & 0x07 ),
                static_cast< std::uint8_t >( aScrollStepInterval & 0x07 ),
                static_cast< std::uint8_t >( aEndPage & 0x07 ),
                static_cast< std::uint8_t >( aVerticalScrollOffset & 0x3F ) };
        return SendCommands( commands );
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

        assert( aNumberOfRowsInTopFixedArea <= 0x3F );
        assert( aNumberOfRowsInScrollArea <= 0x7F );

        const std::uint8_t commands[] = {
            KSetVerticalScrollArea,
            static_cast< std::uint8_t >( aNumberOfRowsInTopFixedArea & 0x3F ),
            static_cast< std::uint8_t >( aNumberOfRowsInScrollArea & 0x7F ),
        };

        return SendCommands( commands );
    }

    // Addressing Setting Command
    TErrorCode inline SetLowerColumnStartAddress( std::uint8_t aStartAddress ) NOEXCEPT
    {
        assert( aStartAddress <= 0x0F );
        return SendCommand( static_cast< std::uint8_t >( aStartAddress & 0x0F ) );
    }

    TErrorCode inline SetHigherColumnStartAddress( std::uint8_t aStartAddress ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdCSetLowerColumnStartAddress = 0x10;
        assert( aStartAddress <= 0x0F );
        return SendCommand(
            static_cast< std::uint8_t >( KCmdCSetLowerColumnStartAddress | aStartAddress & 0x0F ) );
    }

    // Scrolling Command
    enum TMemoryAddressingMode
    {
        HorizontalAddressingMode = 0x00,
        VerticalAddressingMode = 0x01,
        PageAddressingMode = 0x02
    };

    TErrorCode inline SetMemoryAddressingMode(
        TMemoryAddressingMode aMemoryAddressingMode
        = TMemoryAddressingMode::PageAddressingMode ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetMemoryAddressingMode = 0x20;

        const std::uint8_t commands[]
            = { KCmdSetMemoryAddressingMode, static_cast< std::uint8_t >( aMemoryAddressingMode ) };

        return SendCommands( commands );
    }

    TErrorCode
    SetColumnAddress( std::uint8_t aColumnStartAddress, std::uint8_t aColumnLastAddress ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetColumnAddress = 0x21;

        assert( aColumnStartAddress < KMaxColumns );
        assert( aColumnLastAddress < KMaxColumns );

        const std::uint8_t commands[] = {
            KCmdSetColumnAddress,
            static_cast< std::uint8_t >( aColumnStartAddress & 0x7F ),
            static_cast< std::uint8_t >( aColumnLastAddress & 0x7F ),
        };

        return SendCommands( commands );
    }

    TErrorCode
    SetPageAddress( std::uint8_t aPageStartAddress, std::uint8_t aPageLastAddress ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetColumnAddress = 0x22;

        assert( aPageStartAddress < KMaxPages );
        assert( aPageLastAddress < KMaxPages );

        const std::uint8_t commands[] = {
            KCmdSetColumnAddress,
            static_cast< std::uint8_t >( aPageStartAddress & 0x07 ),
            static_cast< std::uint8_t >( aPageLastAddress & 0x07 ),
        };

        return SendCommands( commands );
    }

    TErrorCode inline SetPageStartAddress( std::uint8_t aPageStartAddress ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdPageStartAddress = 0xB0;
        assert( aPageStartAddress <= 0x07 );
        return SendCommand(
            static_cast< std::uint8_t >( KCmdPageStartAddress | aPageStartAddress & 0x07 ) );
    }

    // Hardware Configuration (Panel resolution & layout related) Command
    TErrorCode inline SetDisplayStartLine( std::uint8_t aDisplayStartLine ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdSetDisplayStartLine = 0x40;
        assert( aDisplayStartLine <= 0x3F );
        return SendCommand(
            static_cast< std::uint8_t >( KCmdSetDisplayStartLine | aDisplayStartLine & 0x3F ) );
    }

    TErrorCode inline SetSegmentRemap( bool aSegmentRemapEnabled = false ) NOEXCEPT
    {
        constexpr std::uint8_t KSegmentRemapDisabled = 0xA0;  // Column address 0 is mapped to SEG0
        constexpr std::uint8_t KSegmentRemapEnabled = 0xA1;  // Column address 127 is mapped to SEG0
        return SendCommand( aSegmentRemapEnabled ? KSegmentRemapEnabled : KSegmentRemapDisabled );
    }

    TErrorCode
    SetMultiplexRatio( std::uint8_t aMultiplexRatio ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetMultiplexRatio = 0xA8;

        assert( aMultiplexRatio <= 0x3F );

        const std::uint8_t commands[]
            = { KCmdSetMultiplexRatio, static_cast< std::uint8_t >( aMultiplexRatio & 0x3F ) };

        return SendCommands( commands );
    }

    /// @brief COM Output Scan Direction
    enum class TOutputScanDirection
    {
        ForwardScanDirection
        = 0xC0,  // Normal mode (RESET) Scan from COM0 to COM[N –1], where N is the Multiplex ratio.
        ReverseDirectionScan
        = 0xC8  // Remapped mode. Scan from COM[N-1] to COM0], where N is the Multiplex ratio.
    };

    TErrorCode inline SetCOMOutputScanDirection(
        TOutputScanDirection aOutputScanDirection
        = TOutputScanDirection::ForwardScanDirection ) NOEXCEPT
    {
        return SendCommand( static_cast< std::uint8_t >( aOutputScanDirection ) );
    }

    /**
     * @brief Set vertical shift by COM from 0d~63d.
     * The value is reset to 00h after RESET.
     *
     * @param aMultiplexRatio Vertical shift by COM from 0d~63d.
     * @return TErrorCode KOk if succeed, otherwise appropriate error code
     */
    TErrorCode
    SetDisplayOffset( std::uint8_t aDisplayOffset ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetDisplayOffset = 0xD3;

        assert( aDisplayOffset <= 0x3F );

        const std::uint8_t commands[]
            = { KCmdSetDisplayOffset, static_cast< std::uint8_t >( aDisplayOffset & 0x3F ) };

        return SendCommands( commands );
    }

    TErrorCode
    SetCOMPinsHardwareConfiguration( bool aAlternativeCOMPinConfiguration = true,
                                     bool aEnableCOMLeftRightRemap = false ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetCOMPinsHardwareConfiguration = 0xDA;
        constexpr std::uint8_t KSequentialCOMPinConfiguration = 0x02;
        constexpr std::uint8_t KAlternativeCOMpinConfiguration = 0x12;
        constexpr std::uint8_t KDisableCOMLeftRightRemap = 0x02;
        constexpr std::uint8_t KEnableCOMLeftRightRemap = 0x22;

        const std::uint8_t commands[] = {
            KCmdSetCOMPinsHardwareConfiguration,
            static_cast< std::uint8_t >(
                ( aEnableCOMLeftRightRemap ? KEnableCOMLeftRightRemap : KDisableCOMLeftRightRemap )
                | ( aAlternativeCOMPinConfiguration ? KAlternativeCOMpinConfiguration
                                                    : KSequentialCOMPinConfiguration ) ) };

        return SendCommands( commands );
    }

    // Timing & Driving Scheme Setting Command
    TErrorCode
    SetDisplayClock( std::uint8_t aOscillatorFrequency = 0x08,
                     std::uint8_t aDivideRatio = 1 ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetDisplayClock = 0xD5;

        assert( aDivideRatio <= 0x0F );
        assert( aOscillatorFrequency <= 0x0F );
        assert( aDivideRatio != 0x00 );

        const std::uint8_t commands[] = {
            KCmdSetDisplayClock, static_cast< std::uint8_t >( ( aOscillatorFrequency << 4 )
                                                              | ( ( aDivideRatio - 1 ) & 0x0F ) ) };

        return SendCommands( commands );
    }

    TErrorCode
    SetPreChargePeriod( std::uint8_t aPhase1Period = 0x02,
                        std::uint8_t aPhase2Period = 0x02 ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetPreChargePeriod = 0xD9;

        assert( aPhase1Period <= 0x0F );
        assert( aPhase2Period <= 0x0F );

        const std::uint8_t commands[]
            = { KCmdSetPreChargePeriod,
                static_cast< std::uint8_t >( ( aPhase2Period << 4 ) | ( aPhase1Period & 0x0F ) ) };

        return SendCommands( commands );
    }

    enum class TVCOMHDeselectLevel
    {
        Level_0_65Vcc = 0x00,
        Level_0_77Vcc = 0x20,  // (RESET)
        Level_0_83Vcc = 0x30
    };

    TErrorCode
    SetVCOMHDeselectLevel( TVCOMHDeselectLevel aDeselectLevel
                           = TVCOMHDeselectLevel::Level_0_77Vcc ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdSetVCOMHDeselectLevel = 0xDB;

        const std::uint8_t commands[]
            = { KCmdSetVCOMHDeselectLevel, static_cast< std::uint8_t >( aDeselectLevel ) };

        return SendCommands( commands );
    }

    TErrorCode inline Nop( ) NOEXCEPT
    {
        constexpr std::uint8_t KCmdNop = 0xE3;
        return SendCommand( KCmdNop );
    }

    // Charge Pump Command
    TErrorCode
    EnablePumpSettings( bool aEnable = false ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        constexpr std::uint8_t KCmdEnablePumpSettings = 0x8D;
        constexpr std::uint8_t KEnable = 0x14;
        constexpr std::uint8_t KDisable = 0x10;

        const std::uint8_t commands[] = { KCmdEnablePumpSettings, aEnable ? KEnable : KDisable };

        return SendCommands( commands );
    }

    // Read commands

    AbstractPlatform::TErrorCode
    SendCommand( uint8_t aCommand, bool aNoStop = false ) NOEXCEPT
    {
        // I2C write process expects a control byte followed by data
        // this "data" can be a command or data to follow up a command
        // Co = 1, D/C = 0 => the driver expects a command
        constexpr std::uint8_t controlByte = 0x80;

        if ( iI2CBus.WriteRegisterRaw( iDeviceAddress, controlByte, aCommand ) )
        {
            return AbstractPlatform::KGenericError;
        }
        return AbstractPlatform::KOk;
    }

    AbstractPlatform::TErrorCode
    SendCommands( const uint8_t* aCommands, size_t aCommandsNumber ) NOEXCEPT
    {
        AbstractPlatform::TErrorCode result = AbstractPlatform::KOk;
        for ( size_t i = 0; i < aCommandsNumber; i++ )
        {
            result = SendCommand( aCommands[ i ], ( i + 1 ) < aCommandsNumber );
            if ( result != AbstractPlatform::KOk )
            {
                break;
            }
        }
        return result;
    }

    template < size_t taArrayElemets >
    AbstractPlatform::TErrorCode
    SendCommands( const uint8_t ( &aCommands )[ taArrayElemets ] ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        return SendCommands( aCommands, taArrayElemets );
    }

    inline AbstractPlatform::TErrorCode
    SendRawBuffer( const uint8_t* aDataBuffer, size_t aBufferSize, bool aNoStop = false ) NOEXCEPT
    {
        assert( aDataBuffer != nullptr );

        return iI2CBus.Write( iDeviceAddress, aDataBuffer, aBufferSize, aNoStop ) == aBufferSize
                   ? AbstractPlatform::KOk
                   : AbstractPlatform::KGenericError;
    }

    AbstractPlatform::TErrorCode
    ClearRam( ) NOEXCEPT
    {
        TErrorCode result = AbstractPlatform::KOk;
        result = SetColumnAddress( 0, KMaxColumns - 1 );
        if ( result != AbstractPlatform::KOk )
        {
            return result;
        }
        result = SetPageAddress( 0, KMaxPages - 1 );
        if ( result != AbstractPlatform::KOk )
        {
            return result;
        }

        /* clear screen RAM */
        std::vector< std::uint8_t > dataBuffer( sizeof( KCmdSetRamBuffer ) + KRamSize, 0 );
        dataBuffer[ 0 ] = KCmdSetRamBuffer;
        return SendRawBuffer( dataBuffer.data( ), dataBuffer.size( ), false );
    }

private:
    /* data */
    AbstractPlatform::CI2CBus iI2CBus;
    const std::uint8_t iDeviceAddress;
};

template < typename taDisplayType >
class CSsd1306Hal
{
};

template <>
class CSsd1306Hal< Ssd1306128x32 > : public CSsd1306HalBase< Ssd1306128x32 >
{
public:
    using TErrorCode = AbstractPlatform::TErrorCode;
    using TBase = CSsd1306HalBase< Ssd1306128x32 >;
    using TBase::CSsd1306HalBase;

    TErrorCode
    Init( ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        RETURN_ON_ERROR( DisplayEnable( false ) );
        RETURN_ON_ERROR(
            SetMemoryAddressingMode( TMemoryAddressingMode::HorizontalAddressingMode ) );
        RETURN_ON_ERROR( SetDisplayStartLine( 0 ) );
        RETURN_ON_ERROR( SetSegmentRemap( true ) );  //+
        RETURN_ON_ERROR( SetMultiplexRatio( KPixelHight - 1 ) );
        RETURN_ON_ERROR( SetCOMOutputScanDirection( TOutputScanDirection::ReverseDirectionScan ) );
        RETURN_ON_ERROR( SetDisplayOffset( 0 ) );

        // set COM (common) pins hardware configuration. Board specific
        // magic number. 0x02 Works for 128x32 (false, false), 0x12 Possibly works for
        // 128x64. Other options 0x22, 0x32
        RETURN_ON_ERROR( SetCOMPinsHardwareConfiguration( false, false ) );
        RETURN_ON_ERROR( SetDisplayClock( 0x08, 0x01 ) );
        RETURN_ON_ERROR( SetPreChargePeriod( 0x01, 0x0F ) );
        RETURN_ON_ERROR( SetVCOMHDeselectLevel( TVCOMHDeselectLevel::Level_0_83Vcc ) );

        /* display */
        RETURN_ON_ERROR( SetContrast( 0xFF ) );
        RETURN_ON_ERROR( InverseDisplay( false ) );
        RETURN_ON_ERROR( EnablePumpSettings( true ) );
        RETURN_ON_ERROR( DeactivateScroll( ) );
        RETURN_ON_ERROR( ClearRam( ) );
        RETURN_ON_ERROR( EnableFillWholeRamWith( false ) );
        RETURN_ON_ERROR( DisplayEnable( true ) );

        return AbstractPlatform::KOk;
    }
};

template <>
class CSsd1306Hal< Ssd1306128x64 > : public CSsd1306HalBase< Ssd1306128x64 >
{
public:
    using TErrorCode = AbstractPlatform::TErrorCode;
    using TBase = CSsd1306HalBase< Ssd1306128x64 >;
    using TBase::CSsd1306HalBase;

    TErrorCode
    Init( ) NOEXCEPT
    {
        using namespace AbstractPlatform;
        RETURN_ON_ERROR( DisplayEnable( false ) );
        RETURN_ON_ERROR(
            SetMemoryAddressingMode( TMemoryAddressingMode::HorizontalAddressingMode ) );
        RETURN_ON_ERROR( SetDisplayStartLine( 0 ) );
        RETURN_ON_ERROR( SetSegmentRemap( true ) );  //+
        RETURN_ON_ERROR( SetMultiplexRatio( KPixelHight - 1 ) );
        RETURN_ON_ERROR( SetCOMOutputScanDirection( TOutputScanDirection::ReverseDirectionScan ) );
        RETURN_ON_ERROR( SetDisplayOffset( 0 ) );

        // set COM (common) pins hardware configuration. Board specific
        // magic number. 0x02 Works for 128x32 (false, false), 0x12 Possibly works for
        // 128x64. Other options 0x22, 0x32
        RETURN_ON_ERROR( SetCOMPinsHardwareConfiguration( true, false ) );
        RETURN_ON_ERROR( SetDisplayClock( 0x08, 0x01 ) );
        RETURN_ON_ERROR( SetPreChargePeriod( 0x01, 0x0F ) );
        RETURN_ON_ERROR( SetVCOMHDeselectLevel( TVCOMHDeselectLevel::Level_0_83Vcc ) );

        /* display */
        RETURN_ON_ERROR( SetContrast( 0xFF ) );
        RETURN_ON_ERROR( InverseDisplay( false ) );
        RETURN_ON_ERROR( EnablePumpSettings( true ) );
        RETURN_ON_ERROR( DeactivateScroll( ) );
        RETURN_ON_ERROR( ClearRam( ) );
        RETURN_ON_ERROR( EnableFillWholeRamWith( false ) );
        RETURN_ON_ERROR( DisplayEnable( true ) );

        return AbstractPlatform::KOk;
    }
};

}  // namespace Ssd1306
}  // namespace ExternalHardware