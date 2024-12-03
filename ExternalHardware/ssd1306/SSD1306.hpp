#pragma once
#include <AbstractPlatform/common/Platform.hpp>
// #include <AbstractPlatform/common/ArrayHelper.hpp>
#include <AbstractPlatform/i2c/AbstractI2C.hpp>
// #include <AbstractPlatform/output/display/AbstractDisplay.hpp>
#include <AbstractPlatform/common/ErrorCode.hpp>
#include <AbstractPlatform/common/BinaryOperations.hpp>
#include <ExternalHardware/ssd1306/SSD1306_HAL.hpp>

#include <memory>
#include <cstring>
#include <utility>
#include <cmath>

namespace ExternalHardware
{
namespace Ssd1306
{
template < typename taDisplayType = Ssd1306128x32 >
class CSsd1306
{
private:
    using TSsd1306Hal = CSsd1306Hal< taDisplayType >;

public:
    using TPage = typename TSsd1306Hal::TPage;
    using TErrorCode = AbstractPlatform::TErrorCode;

    CSsd1306( AbstractPlatform::IAbstractI2CBus& aI2CBus,
              std::uint8_t aDeviceAddress = TSsd1306Hal::KDefaultAddress ) NOEXCEPT
        : iSsd1306Hal{ aI2CBus, aDeviceAddress }
    {
    }

    inline void
    Init( )
    {
        iSsd1306Hal.Init( );
    }

    class CRenderArea
    {
    public:
        CRenderArea( CRenderArea&& ) = default;

        inline size_t
        PixelWidth( ) const
        {
            return Columns( );
        }

        inline size_t
        PixelHeight( ) const
        {
            return Pages( ) * TSsd1306Hal::KPixelsPerPage;
        }

        void
        SetPixel( size_t aX, size_t aY, bool enable )
        {
            assert( aX < PixelWidth( ) );
            assert( aY < PixelHeight( ) );

            using namespace AbstractPlatform;
            const auto pageIndex = GetPageIndexByPixelCoordinate( aX, aY );
            iBuffer[ pageIndex ]
                = SetBit( iBuffer[ pageIndex ], GetPagePixelBitIndexByPixelYCoordinate( aY ) );
        }

        void
        ClearPixel( size_t aX, size_t aY, bool enable )
        {
            assert( aX < PixelWidth( ) );
            assert( aY < PixelHeight( ) );

            using namespace AbstractPlatform;
            const auto pageIndex = GetPageIndexByPixelCoordinate( aX, aY );
            iBuffer[ pageIndex ]
                = ClearBit( iBuffer[ pageIndex ], GetPagePixelBitIndexByPixelYCoordinate( aY ) );
        }

        void
        TogglePixel( size_t aX, size_t aY, bool enable )
        {
            assert( aX < PixelWidth( ) );
            assert( aY < PixelHeight( ) );

            using namespace AbstractPlatform;
            const auto pageIndex = GetPageIndexByPixelCoordinate( aX, aY );
            iBuffer[ pageIndex ]
                = ToggleBit( iBuffer[ pageIndex ], GetPagePixelBitIndexByPixelYCoordinate( aY ) );
        }

        bool
        GetPixel( size_t aX, size_t aY ) const
        {
            assert( aX < PixelWidth( ) );
            assert( aY < PixelHeight( ) );

            using namespace AbstractPlatform;
            const auto pageIndex = GetPageIndexByPixelCoordinate( aX, aY );
            return CheckBit( iBuffer[ pageIndex ], GetPagePixelBitIndexByPixelYCoordinate( aY ) );
        }

        void
        DrawLine( int aFromX, int aFromY, int aToX, int aToY, bool aPixelOn )
        {
            assert( aFromX < PixelWidth( ) );
            assert( aToX < PixelWidth( ) );
            assert( aFromY < PixelHeight( ) );
            assert( aToY < PixelHeight( ) );

            if ( aToX < aFromX )
            {
                std::swap( aToX, aFromX );
            }
            if ( aToY < aFromY )
            {
                std::swap( aToX, aFromX );
            }

            int dx = std::abs( aToX - aFromX );
            int sx = aFromX < aToX ? 1 : -1;
            int dy = -std::abs( aToY - aFromY );
            int sy = aFromY < aToY ? 1 : -1;
            int err = dx + dy;
            int e2;

            while ( true )
            {
                SetPixel( aFromX, aFromY, aPixelOn );
                if ( aFromX == aToX && aFromY == aToY )
                {
                    break;
                }
                e2 = 2 * err;

                if ( e2 >= dy )
                {
                    err += dy;
                    aFromX += sx;
                }
                if ( e2 <= dx )
                {
                    err += dx;
                    aFromY += sy;
                }
            }
        }

        void
        FillWith( bool aValue = true )
        {
            std::memset( iBuffer.get( ), static_cast< int >( aValue ), GetRawBufferLength( ) );
        }

        inline void
        Clear( )
        {
            FillWith( false );
        }

        void
        SetPage( size_t aColumnIndex, size_t aPageIndex, TPage aPage )
        {
            assert( aColumnIndex < Columns( ) );
            assert( aPageIndex < Pages( ) );

            const auto pageIndex = aPageIndex * Columns( ) + aColumnIndex;
            iBuffer[ pageIndex ] = aPage;
        }

        inline size_t
        GetPageIndexByPixelCoordinate( size_t aX, size_t aY ) const
        {
            return ( aY / TSsd1306Hal::KPixelsPerPage ) * Columns( ) + aX;
        }

        static constexpr inline TPage
        GetPagePixelBitIndexByPixelYCoordinate( size_t aY )
        {
            return aY % TSsd1306Hal::KPixelsPerPage;
        }

        inline size_t
        Columns( ) const NOEXCEPT
        {
            return static_cast< size_t >( iBeginCol - iLastCol ) + 1u;
        }

        inline size_t
        Pages( ) const NOEXCEPT
        {
            return static_cast< size_t >( iBeginPage - iLastPage ) + 1u;
        }

        size_t
        GetRawBufferLength( ) const
        {
            assert( iBeginPage <= iLastPage );
            assert( iBeginCol <= iLastCol );

            const size_t columns = Columns( );
            const size_t pages = Pages( );

            return columns * pages;
        }

        const std::uint8_t*
        RawBuffer( ) const NOEXCEPT
        {
            return iBuffer.get( );
        }

    private:
        friend class CSsd1306;

        CRenderArea( std::uint8_t aBeginCol,
                     std::uint8_t aLastCol,
                     std::uint8_t aBeginPage,
                     std::uint8_t aLastPage )
            : iBeginCol{ aBeginCol }
            , iLastCol{ aLastCol }
            , iBeginPage{ aBeginPage }
            , iLastPage{ aLastPage }
            , iBuffer{ std::make_unique< TPage[] >( GetRawBufferLength( ) ) }
        {
        }

        const std::uint8_t iBeginCol;
        const std::uint8_t iLastCol;
        const std::uint8_t iBeginPage;
        const std::uint8_t iLastPage;
        std::unique_ptr< TPage[] > iBuffer;
    };

    CRenderArea
    CreateRenderArea( std::uint8_t aBeginCol = 0,
                      std::uint8_t aLastCol = TSsd1306Hal::KMaxColumns - 1,
                      std::uint8_t aBeginPage = 0,
                      std::uint8_t aLastPag = TSsd1306Hal::KMaxPages - 1 )
    {
        assert( aBeginCol < TSsd1306Hal::KMaxColumns );
        assert( aLastCol < TSsd1306Hal::KMaxColumns );
        assert( aBeginPage < TSsd1306Hal::KMaxPages );
        assert( aLastPag < TSsd1306Hal::KMaxPages );

        return CRenderArea( aBeginCol, aLastCol, aBeginPage, aLastPag );
    }

    void
    Render( const CRenderArea& aArea )
    {
        using namespace AbstractPlatform;
        const auto bufferSize = aArea.GetRawBufferLength( );

        iSsd1306Hal.SetColumnAddress( aArea.iBeginCol, aArea.iLastCol );
        iSsd1306Hal.SetPageAddress( aArea.iBeginPage, aArea.iLastPage );

        iSsd1306Hal.SendBuffer( aArea.RawBuffer( ), bufferSize );
    }

private:
    TSsd1306Hal iSsd1306Hal;
};
}  // namespace Ssd1306
}  // namespace ExternalHardware