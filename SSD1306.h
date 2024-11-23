#include <cstdint>
#include <cstring>
#include "Platform.hpp"
#include "AbstractI2C.hpp"

#ifdef __EXCEPTIONS
#define NOEXCEPT noexcept
#include <exception>
#else
#define NOEXCEPT
#endif

namespace ExternalDevice
{

class CSsd1306
{
public:
    static constexpr int KOk = 0;
    static constexpr int KGenericError = -1;
    static constexpr int KInvalidArgumentError = -2;
    static constexpr int KInvalidVendor = -3;

#ifdef __EXCEPTIONS
    template < int taError, const char* const taDescription >
    class EBase : public std::exception
    {
    public:
        constexpr int
        error( ) const NOEXCEPT
        {
            return taError;
        }

        const char*
        what( ) const NOEXCEPT override
        {
            return taDescription;
        }
    };
    static constexpr char KGenericErrorDescription[] = "Internal error";
    static constexpr char KInvalidArgumentDescription[] = "Invalid argument";
    static constexpr char KInvalidVendorDescription[] = "Invalid vendor";
    using EGenericError = EBase< KGenericError, KGenericErrorDescription >;
    using EInvalidArgumentError = EBase< KInvalidArgumentError, KInvalidArgumentDescription >;
    using EInvalidVendorError = EBase< KInvalidVendor, KInvalidVendorDescription >;

#endif

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

    inline int
    Reset( ) NOEXCEPT
    {
        // TODO: implement
        return KOk;
    }

    inline int
    Reset( CConfig aConfig ) NOEXCEPT
    {
        // TODO: implement
        return KOk;
    }

    int GetConfig( CConfig& aConfig ) NOEXCEPT;
    int SetConfig( const CConfig& aConfig ) NOEXCEPT;

#ifdef __EXCEPTIONS

#endif

private:
    /* data */
    AbstractPlatform::IAbstractI2CBus& iI2CBus;
    const std::uint8_t iDeviceAddress;

#ifdef __EXCEPTIONS
    static inline int
    ThrowOnError( int aErrorCode )
    {
        switch ( aErrorCode )
        {
        case KOk:
            return aErrorCode;
            break;
        case KInvalidArgumentError:
            throw EInvalidArgumentError{ };
            break;
        case KInvalidVendor:
            throw EInvalidVendorError{ };
            break;
        case KGenericError:
        default:
            throw EGenericError{ };
            break;
        }
        return aErrorCode;
    }
#endif
};

}  // namespace ExternalDevice