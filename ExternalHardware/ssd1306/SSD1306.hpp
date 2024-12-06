#pragma once
#include <AbstractPlatform/common/Platform.hpp>
#include <AbstractPlatform/common/ErrorCode.hpp>
#include <AbstractPlatform/common/BinaryOperations.hpp>
#include <AbstractPlatform/i2c/AbstractI2C.hpp>
#include <AbstractPlatform/output/display/AbstractDisplay.hpp>
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
    using TDrawer = AbstractPlatform::CDrawer< AbstractPlatform::BitPixel >;
    using TAbstractCanvas = typename TDrawer::TAbstractCanvas;

    CSsd1306( AbstractPlatform::IAbstractI2CBus& aI2CBus,
              std::uint8_t aDeviceAddress = TSsd1306Hal::KDefaultAddress ) NOEXCEPT
        : iSsd1306Hal{ aI2CBus, aDeviceAddress }
    {
    }

    inline TErrorCode
    Init( )
    {
        return iSsd1306Hal.Init( );
    }

    class CRenderArea : public TAbstractCanvas
    {
    public:
        using TPixel = typename TAbstractCanvas::TPixel;

        CRenderArea( CRenderArea&& ) = default;
        virtual ~CRenderArea( ) = default;

        int
        PixelWidth( ) const override
        {
            return GetPixelWidth( );
        }

        int
        PixelHeight( ) const override
        {
            return GetPixelHeight( );
        }

        void
        SetPosition( int aX, int aY ) override
        {
            assert( aX >= 0 );
            assert( aY >= 0 );
            assert( aX < GetPixelWidth( ) );
            assert( aY < GetPixelHeight( ) );

            iCurrentPageIndex = GetPageIndexByPixelCoordinate( aX, aY );
            iCurrentPagePixelBitIndex = GetPagePixelBitIndexByPixelYCoordinate( aY );
        }

        Position
        GetPosition( ) const override
        {
            const size_t x = iCurrentPageIndex % iColumns;
            const size_t y = iCurrentPageIndex / iColumns * TSsd1306Hal::KPixelsPerPage
                             + iCurrentPagePixelBitIndex;
            const Position result{ static_cast< int >( x ), static_cast< int >( y ) };
            return result;
        }

        void
        SetPixel( TPixel aPixelValue ) override
        {
            using namespace AbstractPlatform;
            auto& page = DisplayBuffer( )[ iCurrentPageIndex ];
            page = aPixelValue.iPixelValue ? SetBit( page, iCurrentPagePixelBitIndex )
                                           : ClearBit( page, iCurrentPagePixelBitIndex );
        }

        void
        InvertPixel( ) override
        {
            using namespace AbstractPlatform;
            auto& page = DisplayBuffer( )[ iCurrentPageIndex ];
            page = ToggleBit( page, iCurrentPagePixelBitIndex );
        }

        TPixel
        GetPixel( ) const override
        {
            using namespace AbstractPlatform;
            const auto page = DisplayBuffer( )[ iCurrentPageIndex ];
            return TPixel{ CheckBit( page, iCurrentPagePixelBitIndex ) };
        }

        void
        FillWith( TPixel aValue = TPixel{ } ) override
        {
            std::memset( DisplayBuffer( ), aValue.iPixelValue ? 0xFF : 0x00,
                         GetDisplayBufferSize( ) );
        }

        void
        SetPage( size_t aColumnIndex, size_t aPageIndex, TPage aPage )
        {
            assert( aColumnIndex < iColumns );
            assert( aPageIndex < iRows );

            const auto pageIndex = aPageIndex * iColumns + aColumnIndex;
            auto& page = DisplayBuffer( )[ pageIndex ];
            page = aPage;
        }

        constexpr inline size_t
        Columns( ) const NOEXCEPT
        {
            return iColumns;
        }

        constexpr inline size_t
        Rows( ) const NOEXCEPT
        {
            return iRows;
        }

        constexpr inline size_t
        GetDisplayBufferSize( ) const
        {
            return iColumns * iRows;
        }

        const std::uint8_t*
        DisplayBuffer( ) const NOEXCEPT
        {
            return iBuffer.get( ) + GetControlCommandLength( );
        }

    private:
        friend class CSsd1306;

        CRenderArea( std::uint8_t aBeginColumn,
                     std::uint8_t aLastColumn,
                     std::uint8_t aBeginPage,
                     std::uint8_t aLastPage )
            : iBeginColumn{ aBeginColumn }
            , iLastColumn{ aLastColumn }
            , iBeginPage{ aBeginPage }
            , iLastPage{ aLastPage }
            , iColumns{ Columns( aBeginColumn, aLastColumn ) }
            , iRows{ Rows( aBeginPage, aLastPage ) }
            , iCurrentPagePixelBitIndex{ 0 }
            , iCurrentPageIndex{ 0 }
            , iBuffer{ std::make_unique< TPage[] >( RawBufferSize( ) ) }
        {
            iBuffer[ 0 ] = TSsd1306Hal::KCmdSetRamBuffer;
        }

        constexpr inline int
        GetPixelWidth( ) const
        {
            return static_cast< int >( iColumns );
        }

        constexpr inline int
        GetPixelHeight( ) const
        {
            return static_cast< int >( iRows * TSsd1306Hal::KPixelsPerPage );
        }

        constexpr static inline size_t
        GetControlCommandLength( )
        {
            return sizeof( TSsd1306Hal::KCmdSetRamBuffer );
        }

        inline size_t
        GetPageIndexByPixelCoordinate( size_t aX, size_t aY ) const
        {
            return GetPageIndexByPixelCoordinate( iColumns, aX, aY );
        }

        static constexpr inline TPage
        GetPagePixelBitIndexByPixelYCoordinate( size_t aY )
        {
            return aY % TSsd1306Hal::KPixelsPerPage;
        }

        static constexpr inline std::uint8_t
        Columns( std::uint8_t aBeginColumn, std::uint8_t aLastColumn )
        {
            return static_cast< std::uint8_t >( aLastColumn - aBeginColumn + 1u );
        }

        static constexpr inline std::uint8_t
        Rows( std::uint8_t aBeginPage, std::uint8_t aLastPage )
        {
            return static_cast< std::uint8_t >( aLastPage - aBeginPage + 1u );
        }

        static constexpr inline size_t
        GetPageIndexByPixelCoordinate( size_t aColumns, size_t aX, size_t aY )
        {
            return ( aY / TSsd1306Hal::KPixelsPerPage ) * aColumns + aX;
        }

        std::uint8_t*
        DisplayBuffer( ) NOEXCEPT
        {
            return iBuffer.get( ) + GetControlCommandLength( );
        }
        inline std::uint8_t*
        RawBuffer( ) const NOEXCEPT
        {
            return iBuffer.get( );
        }

        inline size_t
        RawBufferSize( ) const
        {
            return GetControlCommandLength( ) + GetDisplayBufferSize( );
        }

        const std::uint8_t iBeginColumn;
        const std::uint8_t iLastColumn;
        const std::uint8_t iBeginPage;
        const std::uint8_t iLastPage;
        const std::uint8_t iColumns;
        const std::uint8_t iRows;
        std::uint8_t iCurrentPagePixelBitIndex;
        size_t iCurrentPageIndex;

        std::unique_ptr< TPage[] > iBuffer;
    };

    CRenderArea
    CreateRenderArea( std::uint8_t aBeginColumn = 0,
                      std::uint8_t aLastColumn = TSsd1306Hal::KMaxColumns - 1,
                      std::uint8_t aBeginPage = 0,
                      std::uint8_t aLastPage = TSsd1306Hal::KMaxPages - 1 )
    {
        assert( aBeginColumn < TSsd1306Hal::KMaxColumns );
        assert( aLastColumn < TSsd1306Hal::KMaxColumns );
        assert( aBeginPage < TSsd1306Hal::KMaxPages );
        assert( aLastPage < TSsd1306Hal::KMaxPages );

        return CRenderArea( aBeginColumn, aLastColumn, aBeginPage, aLastPage );
    }

    static constexpr TDrawer
    CreateDrawer( CRenderArea& aRenderArea )
    {
        return TDrawer( aRenderArea );
    }

    void
    Render( const CRenderArea& aRenderArea )
    {
        using namespace AbstractPlatform;
        const auto rawBufferSize = aRenderArea.RawBufferSize( );
        assert( rawBufferSize != 0u );
        assert( aRenderArea.RawBuffer( ) != nullptr );

        iSsd1306Hal.SetColumnAddress( aRenderArea.iBeginColumn, aRenderArea.iLastColumn );
        iSsd1306Hal.SetPageAddress( aRenderArea.iBeginPage, aRenderArea.iLastPage );

        iSsd1306Hal.SendRawBuffer( aRenderArea.RawBuffer( ), rawBufferSize );
    }

private:
    TSsd1306Hal iSsd1306Hal;
};
}  // namespace Ssd1306
}  // namespace ExternalHardware