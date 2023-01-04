#include "stdafx.h"
#include <Windows.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <chrono>
#include <WtsApi32.h>
#include <Psapi.h>
#include <curl/curl.h>

#pragma comment( lib, "libcurl.lib" )
#pragma comment( lib, "WtsApi32.lib" )

namespace str
{
	// transform data
	std::string ToA ( const std::wstring& _s )
	{
		const auto num_chars = WideCharToMultiByte ( CP_UTF8 , 0 , _s.data ( ) , int ( _s.length ( ) ) , nullptr , 0 , nullptr , nullptr );
		const auto strTo = static_cast< CHAR* >( malloc ( ( num_chars + 1 ) * sizeof ( CHAR ) ) );
		if ( strTo )
		{
			WideCharToMultiByte ( CP_UTF8 , 0 , _s.data ( ) , int ( _s.length ( ) ) , strTo , num_chars , nullptr , nullptr );
			strTo [ num_chars ] = '\0';
		}
		return strTo;
	}
	std::wstring ToW ( const std::string& _s )
	{
		const auto num_chars = MultiByteToWideChar ( CP_UTF8 , 0 , _s.data ( ) , int ( _s.length ( ) ) , nullptr , 0 );
		const auto wstrTo = static_cast< WCHAR* >( malloc ( ( num_chars + 1 ) * sizeof ( WCHAR ) ) );
		if ( wstrTo )
		{
			MultiByteToWideChar ( CP_UTF8 , 0 , _s.data ( ) , int ( _s.length ( ) ) , wstrTo , num_chars );
			wstrTo [ num_chars ] = L'\0';
		}
		return wstrTo;
	}

	int ToInt ( const std::string& _data )
	{
		return std::stoi ( _data );
	}
	bool ToBool ( const std::string& _data )
	{
		return ( std::stoi ( _data ) > 0 );
	}
	float ToFloat ( const std::string& _data )
	{
		return std::strtof ( _data.data ( ) , 0 );
	}

	std::string FromInt ( int _i )
	{
		return std::to_string ( _i );
	}
	std::string FromBool ( bool _b )
	{
		return _b ? "true" : "false";
	}
	std::string FromFloat ( float _f , int _prec )
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision ( _prec ) << _f;
		return stream.str ( );
	}

	// find , trim , replace
	std::string TrimSpaces ( const std::string& _s )
	{
		return TrimRightSpaces ( TrimLeftSpaces ( _s ) );
	}
	std::string TrimLeftSpaces ( const std::string& _s )
	{
		std::string buffer = _s;
		auto it2 = std::find_if ( buffer.begin ( ) , buffer.end ( ) , [ ] ( char ch ) { return !std::isspace<char> ( ch , std::locale::classic ( ) ); } );
		buffer.erase ( buffer.begin ( ) , it2 );
		return buffer;
	}
	std::string TrimRightSpaces ( const std::string& _s )
	{
		std::string buffer = _s;
		auto it1 = std::find_if ( buffer.rbegin ( ) , buffer.rend ( ) , [ ] ( char ch ) { return !std::isspace<char> ( ch , std::locale::classic ( ) ); } );
		buffer.erase ( it1.base ( ) , buffer.end ( ) );
		return buffer;
	}
	std::string ReplaceDots ( const std::string& _s )
	{
		size_t pos;
		std::string buffer = _s;
		while ( ( pos = buffer.find ( "." ) ) != std::string::npos )
		{
			buffer.replace ( pos , 1 , "" );
		}
		return buffer;
	}
	std::string ReplaceCmdSpaces ( const std::string& _s )
	{
		std::string input = _s;
		for ( std::size_t i = 0; i < input.length ( ); i++ )
		{
			if ( input [ i ] == ' ' )
			{
				input.replace ( i , 1 , "\ " );
			}
		}
		return input;

	}
	std::string FindLastTrimLeft ( const std::string& _s , const std::string& _find )
	{
		const auto idx = _s.rfind ( _find );
		if ( std::string::npos == idx )
		{
			return _s;
		}
		return _s.substr ( idx + _find.size ( ) );
	}
	std::string FindFirstTrimLeft ( const std::string& _s , const std::string& _find )
	{
		const auto idx = _s.find ( _find );
		if ( std::string::npos == idx )
		{
			return _s;
		}
		return _s.substr ( idx + _find.size ( ) );
	}
	std::string FindLastTrimRight ( const std::string& _s , const std::string& _find )
	{
		const auto idx = _s.rfind ( _find );
		if ( std::string::npos == idx )
		{
			return _s;
		}
		return _s.substr ( 0 , idx );
	}
	std::string FindFirstTrimRight ( const std::string& _s , const std::string& _find )
	{
		const auto idx = _s.find ( _find );
		if ( std::string::npos == idx )
		{
			return _s;
		}
		return _s.substr ( 0 , idx );
	}
	std::string Find ( const std::string& _s , const std::string& _left , const std::string& _right )
	{
		return FindFirstTrimRight ( FindFirstTrimLeft ( _s , _left ) , _right );
	}

	// time , hour formatted
	std::string HourMinSec ( const std::string& _space )
	{
		SYSTEMTIME systime;
		GetLocalTime ( &systime );
		char Buffer [ MAX_PATH ] = { 0 };
		const std::string buffer = "%.2d" + _space + "%.2d" + _space + "%.2d";
		sprintf_s ( Buffer , buffer.data ( ) , systime.wHour , systime.wMinute , systime.wSecond );
		return std::string ( Buffer );
	}
	std::string DayMonthYear ( const std::string& _space )
	{
		SYSTEMTIME systime;
		GetLocalTime ( &systime );
		char Buffer [ MAX_PATH ] = { 0 };
		const std::string buffer = "%.2d" + _space + "%.2d" + _space + "%.2d";
		sprintf_s ( Buffer , buffer.data ( ) , systime.wDay , systime.wMonth , systime.wYear );
		return std::string ( Buffer );
	}

	// commands
	std::string WinConsoleCmd ( const std::string& _cmd )
	{
		std::cout << "[cmd] " << _cmd << std::endl;
		std::string result;
		std::array<char , 1024> buffer = { };
		std::unique_ptr<FILE , decltype( &_pclose )> pipe ( _popen ( _cmd.data ( ) , "r" ) , _pclose );
		while ( fgets ( buffer.data ( ) , int ( buffer.size ( ) ) , pipe.get ( ) ) != nullptr )
		{
			result += buffer.data ( );
		}
		return result;
	}
	std::string PowerShellCmd ( const std::string& _cmd )
	{
		std::string temp_path = util::RandomString ( 4 , 8 ) + ".ps1";
		if ( file::local::Exist ( temp_path ) )
		{
			temp_path = util::RandomString ( 4 , 8 ) + ".ps1";
			if ( file::local::Exist ( temp_path ) )
			{
				file::local::Remove ( temp_path );
			}
		}
		std::ofstream file;
		file.open ( temp_path );
		std::string newArg = "-auto";
		std::string powershell;
		powershell = "$task = Get-ScheduledTask " + _cmd + "\n";
		powershell += "$items = @{}\n";
		powershell += "if ($task.Actions.Execute -ne $null) {$items.Add('Execute', \"$($usertask.Actions.Execute)\")} \n";
		powershell += "$items.Add('Argument', \"$($task.Actions.Arguments) " + newArg + "\") \n"; // 'Argument' not a typo
		powershell += "if ($task.Actions.WorkingDirectory -ne $null) {$items.Add('WorkingDirectory',\"$($task.Actions.WorkingDirectory)\")} \n";
		powershell += "$action = New-ScheduledTaskAction @items\n";
		powershell += "$task.Actions = $action\n";
		powershell += "Set-ScheduledTask -InputObject $task\n";
		file << powershell << std::endl;
		file.close ( );
		const auto buffer = WinConsoleCmd ( "powershell -ExecutionPolicy Bypass -File " + temp_path + " runas /user:administrator" );
		file::local::Remove ( temp_path );
		return buffer;
	}
}

namespace util
{
	int RandomInt ( int _min , int _max )
	{
		std::random_device rd;
		std::mt19937 gen ( rd ( ) );
		std::uniform_int_distribution<> dist ( _min , _max );
		return dist ( gen );
	}
	float RandomFloat ( float _min , int _max )
	{
		std::random_device rd;
		std::mt19937 gen ( rd ( ) );
		std::uniform_real_distribution<> dis ( _min , _max );
		return ( float ) dis ( gen );
	}
	std::string RandomString ( int _min_len , int _max_len )
	{
		char __alphabet [ ] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0x0 };
		static std::string charset = __alphabet;
		std::string result;
		result.resize ( RandomInt ( _min_len , _max_len ) );
		for ( auto i = 0U; i < result.size ( ); i++ )
		{
			result [ i ] = charset [ rand ( ) % charset.length ( ) ];
		}
		return result;
	}

	void SetClipboard ( const std::string& _s )
	{
		int size = ::MultiByteToWideChar ( CP_UTF8 , 0 , _s.data ( ) , -1 , nullptr , 0 );
		if ( size < 0 )
		{
			return;
		}
		if ( ::OpenClipboard ( NULL ) )
		{
			::EmptyClipboard ( );
			HGLOBAL hGlobal = ::GlobalAlloc ( GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE , ( size + 1 ) * sizeof ( WCHAR ) );
			if ( hGlobal != NULL )
			{
				LPWSTR lpszData = ( LPWSTR )::GlobalLock ( hGlobal );
				if ( lpszData != nullptr )
				{
					::MultiByteToWideChar ( CP_UTF8 , 0 , _s.data ( ) , -1 , lpszData , size );
					::GlobalUnlock ( hGlobal );
					::SetClipboardData ( CF_UNICODETEXT , hGlobal );
				}
			}
			::CloseClipboard ( );
		}
	}

	DWORD GetProcId ( const std::string& _process )
	{
		DWORD pid = 0UL;
		DWORD dwProcCount = 0;
		WTS_PROCESS_INFOA* pWPIs = nullptr;
		if ( WTSEnumerateProcessesA ( WTS_CURRENT_SERVER_HANDLE , NULL , 1 , &pWPIs , &dwProcCount ) )
		{
			const auto hash_name = HashStr ( _process );
			for ( auto i = 0U; i < dwProcCount; i++ )
			{
				const auto lpName = pWPIs [ i ].pProcessName;
				if ( hash_name == HashStr ( lpName ) )
				{
					pid = pWPIs [ i ].ProcessId;
					break;
				}
			}
		}
		if ( pWPIs )
		{
			WTSFreeMemory ( pWPIs );
			pWPIs = 0;
		}
		return pid;
	}
	bool StartProcess ( const std::string& _path , const std::string& _arg , bool _wait )
	{
		STARTUPINFOA si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		ZeroMemory ( &si , sizeof ( si ) );
		si.cb = sizeof ( si );
		ZeroMemory ( &pi , sizeof ( pi ) );
		if ( !CreateProcessA ( _path.data ( ) , ( LPSTR ) _arg.data ( ) , 0 , 0 , 0 , CREATE_NEW_PROCESS_GROUP , 0 , 0 , &si , &pi ) )
		{
			return cLog.Error( "failed to start process: " + _path + " width arg: " + _arg );
		}
		if ( _wait )
		{
			WaitForSingleObject ( pi.hProcess , INFINITE );
		}
		CloseHandle ( pi.hProcess );
		CloseHandle ( pi.hThread );
		return true;
	}
	bool StopProcessById ( DWORD _pid )
	{
		const auto handle = OpenProcess ( PROCESS_VM_OPERATION | PROCESS_TERMINATE , FALSE , _pid );
		if ( handle == INVALID_HANDLE_VALUE )
		{
			return cLog.Error ( "invalid handle for pid: " + _pid );
		}
		if ( TerminateProcess ( handle , 0 ) == FALSE )
		{
			CloseHandle ( handle );
			return cLog.Error ( "failed to terminate process id: " + _pid );
		}
		CloseHandle ( handle );
		return true;
	}
	bool StopProcess ( const std::string& _name )
	{
		const auto pid = GetProcId ( _name );
		if ( pid <= 5UL )
		{
			return cLog.Error ( "couldn't find process while trying to close it: " + _name );
		}
		return StopProcessById ( pid );
	}
	bool StopWindow ( const std::string& _name )
	{
		const auto hWnd = FindWindowA ( 0 , _name.data ( ) );
		if ( !IsWindow ( hWnd ) )
		{
			return cLog.Error ( "failed to find window: " + _name );
		}
		DWORD pid = 0UL;
		GetWindowThreadProcessId ( hWnd , &pid );
		if ( pid <= 5UL )
		{
			return cLog.Error ( "invalid pid for window: " + _name );
		}
		return StopProcessById ( pid );
	}
	void GetScreenSize ( int* _x , int* _y )
	{
		*_x = GetSystemMetrics ( SM_CXSCREEN );
		*_y = GetSystemMetrics ( SM_CYSCREEN );
	}
}

auto operator==( const StdCustomHash_t& lhs , const StdCustomHash_t& rhs )->bool
{
	return lhs.first_name == rhs.first_name;
}

auto StdCustomHash::operator()( const StdCustomHash_t& s ) const noexcept->hash_t
{
	return std::hash<std::string>{}( s.first_name ) ^ ( s.hash_value << 1 );
}

auto hash_string ( const std::string& str )->hash_t
{
	return StdCustomHash {}( str );
}

namespace base64
{
	const char _Alphabet [ ] = {
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
	};
	std::string Encrypt ( const std::string& _str )
	{
		std::string out;
		auto val = 0 , valb = -6;
		for ( auto i = 0U; i < _str.length ( ); ++i )
		{
			const auto c = ( unsigned char ) _str [ i ];
			val = ( val << 8 ) + c;
			valb += 8;
			while ( valb >= 0 )
			{
				out.push_back ( _Alphabet [ ( val >> valb ) & 0x3F ] );
				valb -= 6;
			}
		}
		if ( valb > -6 )
		{
			out.push_back ( _Alphabet [ ( ( val << 8 ) >> ( valb + 8 ) ) & 0x3F ] );
		}
		return out;
	}
	std::string Decrypt ( const std::string& _str )
	{
		std::string out;
		std::vector<int> T ( 256 , -1 );
		for ( auto i = 0; i < 64; i++ )
		{
			T [ _Alphabet [ i ] ] = i;
		}
		auto val = 0 , valb = -8;
		for ( const auto& c : _str )
		{
			if ( T [ c ] == -1 )
			{
				break;
			}
			val = ( val << 6 ) + T [ c ];
			valb += 6;
			if ( valb >= 0 )
			{
				out.push_back ( char ( ( val >> valb ) & 0xFF ) );
				valb -= 8;
			}
		}
		return out;
	}
}