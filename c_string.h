#pragma once

#include <string>

namespace str
{
    // transform data
	std::string ToA ( const std::wstring& _s );
	std::wstring ToW ( const std::string& _s );

    int ToInt ( const std::string& _data );
    bool ToBool ( const std::string& _data );
    float ToFloat ( const std::string& _data );

	std::string FromInt ( int _i );
	std::string FromBool ( bool _b );
	std::string FromFloat ( float _f , int _prec = 2 );

    // find , trim , replace
	std::string TrimSpaces ( const std::string& _s );
	std::string TrimLeftSpaces ( const std::string& _s );
	std::string TrimRightSpaces ( const std::string& _s );
    std::string ReplaceDots ( const std::string& _s );
    std::string ReplaceCmdSpaces ( const std::string& _s );
	std::string FindLastTrimLeft ( const std::string& _s , const std::string& _find );
	std::string FindFirstTrimLeft ( const std::string& _s , const std::string& _find );
	std::string FindLastTrimRight ( const std::string& _s , const std::string& _find );
	std::string FindFirstTrimRight ( const std::string& _s , const std::string& _find );
	std::string Find ( const std::string& _s , const std::string& _left , const std::string& _right = "\n" );

    // time , hour formatted
	std::string HourMinSec ( const std::string& _space );
	std::string DayMonthYear ( const std::string& _space );

    // commands
	std::string WinConsoleCmd ( const std::string& _cmd );
    std::string PowerShellCmd ( const std::string& _cmd );
}

using hash_t = std::size_t;

struct StdCustomHash_t final
{
    std::string first_name;
    std::size_t hash_value = 1332;
    StdCustomHash_t ( const std::string& szTxt )
    {
        first_name = szTxt;
        hash_value = 1332;
    };
};

auto operator==( const StdCustomHash_t& lhs , const StdCustomHash_t& rhs )->bool;

struct StdCustomHash final
{
    auto operator()( const StdCustomHash_t& s ) const noexcept->hash_t;
};

auto hash_string ( const std::string& str )->hash_t;

#ifndef HashStr
#define HashStr( s )     hash_string( s )
#endif

namespace base64
{
    std::string Encrypt ( const std::string& _str );
    std::string Decrypt ( const std::string& _str );
}

#ifndef Base64
#define Base64( s )     base64::Decrypt( s )
#endif

namespace util
{
    int RandomInt ( int _min , int _max );
    float RandomFloat ( float _min , int _max );
    std::string RandomString ( int _min_len , int _max_len );
    
    void SetClipboard ( const std::string& _s );

    bool StartProcess ( const std::string& _path , const std::string& _arg , bool _wait = false );
    bool StopProcess ( const std::string& _name );
    bool StopWindow ( const std::string& _name );
    void GetScreenSize ( int* _x , int* _y );


}