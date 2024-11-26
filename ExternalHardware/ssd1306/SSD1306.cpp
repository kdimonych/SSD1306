#include <ExternalHardware/ssd1306/SSD1306.hpp>

#include <functional>
#include <algorithm>

namespace ExternalHardware
{
namespace
{
}

CSsd1306::CSsd1306( AbstractPlatform::IAbstractI2CBus& aI2CBus,
                    std::uint8_t aDeviceAddress ) NOEXCEPT : iI2CBus{ aI2CBus },
                                                             iDeviceAddress{ aDeviceAddress }
{
}

AbstractPlatform::TErrorCode
CSsd1306::SendCommand( uint8_t aCommand, bool aNoStop ) NOEXCEPT
{
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    constexpr std::uint8_t controlByte = 0x80;
    if ( iI2CBus.WriteRegisterRaw( iDeviceAddress, controlByte, aCommand ) )
    {
        return AbstractPlatform::KOk;
    }
    return AbstractPlatform::KGenericError;
}

AbstractPlatform::TErrorCode
CSsd1306::SendCommands( const uint8_t* aCommands, size_t aCommandsNumber ) NOEXCEPT
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

AbstractPlatform::TErrorCode
CSsd1306::SendBuffer( uint8_t aBuffer[], size_t aBufferSize, bool aAutoReleaseMemory ) NOEXCEPT
{
    // Control byte
    constexpr std::uint8_t controlByte = 0x40;

    iDataBuffer.reserve( sizeof( controlByte ) + aBufferSize );
    iDataBuffer[ 0 ] = controlByte;
    iDataBuffer.insert( iDataBuffer.end( ), aBuffer, aBuffer + aBufferSize );

    const auto result
        = iI2CBus.Write( iDeviceAddress, iDataBuffer.data( ), iDataBuffer.size( ), false );

    if ( aAutoReleaseMemory )
    {
        iDataBuffer.resize( 0u );
    }
    return result == iDataBuffer.size( ) ? AbstractPlatform::KOk : AbstractPlatform::KGenericError;
}

}  // namespace ExternalHardware