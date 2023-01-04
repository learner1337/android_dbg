#include "c_android.h"
#include "c_directx.h"
#include <iostream>

#define ANDROID_SDCARD_PATH			"/sdcard"
#define ANDROID_DCIM_CAMERA_PATH	"/DCIM/Camera"
#define ANDROID_STORAGE_PATH		"/storage/self/primary"
#define ANDROID_DOWNLOAD_PATH		"/Download"
#define SCREEN_AWAKE				2209493530483860916

namespace adb
{
	std::string Command ( const std::string& _id , const std::string& _cmd )
	{
		return str::WinConsoleCmd ( "adb -s " + _id + " " + _cmd );
	}

	bool ServiceFound ( const std::string& _id , const std::string& _service )
	{
		return str::WinConsoleCmd ( "adb -s " + _id + " shell service check " + _service ).find ( ": found" ) == std::string::npos;
	}

	void Restart ( )
	{
		str::WinConsoleCmd ( "adb kill-server" );
	}
	bool Connect ( const std::string& _id )
	{
		return str::WinConsoleCmd ( "adb connect " + _id ).find ( "No such host is known" ) == std::string::npos;
	}
	std::string InterfaceName ( const std::string& _id )
	{
		const auto buffer = Command ( _id , "shell getprop wifi.interface" );
		if ( buffer.empty ( ) )
		{
			return "wlan0";
		}
		return buffer;
	}
	std::string UsbToWifi ( const std::string& _id )
	{
		const auto buffer = str::Find ( Command ( _id , "shell ip addr show " + InterfaceName ( _id ) ) , "inet " , "/" );
		if ( buffer.empty ( ) )
		{
			return "";
		}
		return buffer + ":5555";
	}
	std::string ConnectWifi ( const std::string& _id )
	{
		const auto wifi_address = UsbToWifi ( _id );
		if ( wifi_address.empty ( ) )
		{
			return "";
		}
		Command ( _id , "tcpip 5555" );
		if ( !Connect ( wifi_address ) )
		{
			return "";
		}
		return wifi_address;
	}
	std::vector<std::string> DevicesList ( )
	{
		std::vector<std::string> device;
		std::string buffer = str::WinConsoleCmd ( "adb devices" );

		// check for disconnected device
		if ( buffer.find ( "offline" ) != std::string::npos || buffer.find ( "unauthorized" ) != std::string::npos )
		{
			Restart ( );
			if ( !buffer.empty ( ) )
			{
				buffer.clear ( );
			}
			buffer = str::WinConsoleCmd ( "adb devices" );
		}

		// first device
		const auto first_device = str::TrimSpaces ( str::Find ( buffer , "attached\n" , "device" ) );
		if ( first_device.empty ( ) )
		{
			return device;
		}

		device.push_back ( first_device );
		buffer = str::FindFirstTrimLeft ( buffer , first_device );

		// other device(s)
		while ( buffer.size ( ) > 0U )
		{
			const auto buffer2 = str::TrimSpaces ( str::Find ( buffer , "device\n" , "device" ) );
			if ( buffer2.empty ( ) )
			{
				break;
			}
			device.push_back ( buffer2 );
			buffer = buffer.substr ( buffer.find ( buffer2 ) );
		}
		return device;
	}

	void InstallApp ( const std::string& _id , const std::string& _path )
	{
		Command ( _id , "install " + _path );
	}
	void UninstallApp ( const std::string& _id , const std::string& _path )
	{
		Command ( _id , "uninstall " + _path );
	}
	bool AppExist ( const std::string& _id , const std::string& _app_name )
	{
		const auto buffer = Command ( _id , "shell pm list packages -3" );
		return ( buffer.find ( _app_name ) != std::string::npos );
	}
	bool StartActivity ( const std::string& _id , const std::string& _activity )
	{
		const auto buffer = Command ( _id , "shell am start -n " + _activity );
		return ( buffer.find ( "Error" ) == std::string::npos || buffer.find ( "does not exist" ) != std::string::npos );
	}
	bool StopActivity ( const std::string& _id , const std::string& _activity )
	{
		const auto buffer = Command ( _id , "shell am force-stop " + _activity );
		return ( buffer.find ( "Error" ) == std::string::npos || buffer.find ( "does not exist" ) != std::string::npos );
	}

	void Press ( const std::string& _id , Key_t _key )
	{
		Command ( _id , "shell input keyevent " + std::to_string ( int ( _key ) ) );
	}
	void Touch ( const std::string& _id , int _x , int _y )
	{
		Command ( _id , "shell input tap " + std::to_string ( _x ) + std::string ( " " ) + std::to_string ( _y ) );
	}
	void Insert ( const std::string& _id , const std::string& _txt )
	{
		std::string buffer = "";
		for ( auto i = 0U; i < _txt.size ( ); ++i )
		{
			if ( _txt [ i ] == ' ' )
			{
				buffer += "\\";
			}
			buffer += _txt [ i ];
		}
		Command ( _id , "shell input text " + buffer );
	}

	std::vector<std::string> FileList ( const std::string& _id , const std::string& _directory )
	{
		std::vector<std::string> vBuffer;
		auto buffer = Command ( _id , "shell ls " + _directory );

		while ( buffer.size ( ) > 0U )
		{
			const auto file = str::FindFirstTrimRight ( buffer , "\n" );
			if ( file.empty ( ) )
			{
				break;
			}
			vBuffer.push_back ( file );
			buffer = str::FindFirstTrimLeft ( buffer , "\n" );
		}
		return vBuffer;
	}

	std::string SmsInbox ( const std::string& _id )
	{
		return str::WinConsoleCmd ( "adb.exe -s " + _id + " shell content query --uri content://sms/inbox" );
	}
	int SmsCount ( const std::string& _id )
	{
		std::string buffer = adb::SmsInbox ( _id );
		buffer = str::FindLastTrimLeft ( buffer , "Row:" );
		buffer = str::FindFirstTrimRight ( buffer , " _id" );
		if ( buffer.empty ( ) )
		{
			return -1;
		}
		return std::stoi ( buffer );
	}
	std::string ScreenAwake ( const std::string& _id )
	{
		return str::Find ( Command ( _id , "shell dumpsys power" ) , "mWakefulness=" );
	}
	int BatteryLevel ( const std::string& _id )
	{
		const auto battery_info = Command ( _id , "shell dumpsys battery" );
		if ( battery_info.empty ( ) )
		{
			return 0;
		}
		const auto battery_level = str::Find ( battery_info , "level: " , "\n" );
		if ( battery_level.empty ( ) )
		{
			return 0;
		}
		return std::stoi ( battery_level );
	}
	int MusicVolume ( const std::string& _id )
	{
		std::string buffer = Command ( _id , "shell dumpsys audio" );
		if ( buffer.empty ( ) )
		{
			return -1;
		}
		buffer = str::FindFirstTrimLeft ( buffer , "STREAM_MUSIC:" );
		buffer = str::Find ( buffer , "streamVolume:" );
		if ( buffer.empty ( ) )
		{
			return -1;
		}
		return std::stoi ( buffer );
	}
	State_t ConnectionState ( const std::string& _id )
	{
		std::string adb_devices = str::WinConsoleCmd ( "adb devices" );
		adb_devices = str::FindFirstTrimLeft ( adb_devices , _id );
		adb_devices = str::FindFirstTrimRight ( adb_devices , "\n" );
		if ( adb_devices.empty ( ) )
		{
			return State_t::STATE_UNKNOWN;
		}
		if ( adb_devices.find ( "offline" ) != std::string::npos )
		{
			return State_t::STATE_OFFLINE;
		}
		if ( adb_devices.find ( "unauthorized" ) != std::string::npos )
		{
			return State_t::STATE_UNAUTHORIZED;
		}
		if ( adb_devices.find ( "device" ) != std::string::npos )
		{
			if ( _id.find ( "." ) != std::string::npos )
			{
				return State_t::STATE_WIFI;
			}
			return State_t::STATE_USB;
		}
		return State_t::STATE_UNKNOWN;
	}
	std::string WindowName ( const std::string& _id )
	{
		return str::FindLastTrimRight ( Command ( _id , "shell getprop ro.product.model" ) , "\n" );
	}
	std::string ScreenShot ( const std::string& _id )
	{
		const auto LastPicPath = [ & ] ( )->std::string
		{
			const auto storage_path = ANDROID_STORAGE_PATH + std::string ( "/" ) + ANDROID_DCIM_CAMERA_PATH;
			const auto storage_list = FileList ( _id , storage_path );
			if ( !storage_list.empty ( ) )
			{
				for ( auto i = storage_list.size ( ) - 1; i > 0U; --i )
				{
					if ( storage_list [ i ].find ( ".jpg" ) != std::string::npos )
					{
						return storage_path + storage_list [ i ];
					}
				}
			}

			const auto sdcard_path = ANDROID_SDCARD_PATH + std::string ( "/" ) + ANDROID_DCIM_CAMERA_PATH;
			const auto sdcard_list = FileList ( _id , sdcard_path );
			if ( !sdcard_list.empty ( ) )
			{
				for ( auto i = sdcard_list.size ( ) - 1; i > 0U; --i )
				{
					if ( sdcard_list [ i ].find ( ".jpg" ) != std::string::npos )
					{
						return sdcard_path + sdcard_list [ i ];
					}
				}
			}
			return "";
		};
		Command ( _id , "shell screencap" );
		return LastPicPath ( );
	}
	std::string PhoneNumber ( const std::string& _id )
	{
		std::string result = Command ( _id , "shell service call iphonesubinfo 15" );
		if ( result.empty ( ) )
		{
			return "";
		}
		std::string buffer = "";
		result = str::FindFirstTrimLeft ( result , "...+" );
		if ( result.empty ( ) )
		{
			return "";
		}
		const auto first_part = str::Find ( result , ".1." , ".'" );
		if ( first_part.empty ( ) )
		{
			return "";
		}
		const auto second_part = str::Find ( result , " '" , ".'" );
		if ( second_part.empty ( ) )
		{
			return "";
		}
		std::string num = first_part + second_part;
		return str::ReplaceDots ( num );
	}
	void Resolution ( const std::string& _id , int* _width , int* _height )
	{
		// result: Physical size: 1440x2880
		std::string buffer = Command ( _id , "shell wm size" );

		// width
		const auto width = str::Find ( buffer , "size: " , "x" );
		if ( width.empty ( ) )
		{
			return;
		}
		*_width = std::stoi ( width );

		// height
		const auto height = str::Find ( buffer , "x" );
		if ( height.empty ( ) )
		{
			return;
		}
		*_height = std::stoi ( height );
	}

	void FileRemove ( const std::string& _id , const std::string& _path )
	{
		Command ( _id , "shell rm " + _path );
	}
	bool FileExist ( const std::string& _id , const std::string& _path )
	{
		const auto file_list = FileList ( _id , file::FolderFromPath ( _path ) );
		if ( file_list.empty ( ) )
		{
			return false;
		}
		for ( const auto& file : file_list )
		{
			if ( HashStr ( file ) == HashStr ( _path ) )
			{
				return true;
			}
		}
		return false;
	}
	void FilePull ( const std::string& _id , const std::string& _android_path , const std::string& _pc_path )
	{
		Command ( _id , "pull " + _android_path + std::string ( " " ) + _pc_path );
	}
	void FilePush ( const std::string& _id , const std::string& _pc_path , const std::string& _android_path )
	{
		Command ( _id , "push " + _android_path + std::string ( " " ) + _pc_path );
	}

	void Press ( const std::string& _id , int _percent_x , int _percent_y , int _width , int _height )
	{
		const auto pos_x = _width * ( float ( _percent_x ) * 0.01f );
		const auto pos_y = _height * ( float ( _percent_y ) * 0.01f );
		adb::Touch ( _id , pos_x , pos_y );
	}

	bool BackgroundSms12 ( const std::string& _id , const std::string& _num , const std::string& _txt )
	{
		Command ( _id , "shell service call isms 5 i32 1 s16 \"com.android.mms.service\" s16 \"null\" s16 \"" + _num + "\" s16 \"null\" s16 \"" + str::ReplaceCmdSpaces ( _txt ) + "\" s16 \"null\" s16 \"null\" i32 0 i64 0" );
		return true;
	}
	bool BackgroundSms10 ( const std::string& _id , const std::string& _num , const std::string& _txt )
	{
		Command ( _id , "shell service call isms 5 i32 1 s16 \"com.android.mms.service\" s16 \"null\" s16 \"" + _num + "\" s16 \"null\" s16 \"" + str::ReplaceCmdSpaces ( _txt ) + "\" s16 \"null\" s16 \"null\" i32 0 i64 0" );
		return true;
	}
	bool BackgroundSms8 ( const std::string& _id , const std::string& _num , const std::string& _txt )
	{
		Command ( _id , "shell service call isms 7 i32 0 s16 \"com.android.mms.service\" s16 \"" + _num + "\" s16 \"null\" s16 \"" + str::ReplaceCmdSpaces ( _txt ) + "\" s16 \"null\" s16 \"null\"" );
		return true;
	}
	bool BackgroundSms7 ( const std::string& _id , const std::string& _num , const std::string& _txt )
	{
		Command ( _id , "shell service call isms 7 i32 1 s16 \"com.android.mms\" s16 \"" + _num + "\" s16 \"null\" s16 \"" + str::ReplaceCmdSpaces ( _txt ) + "\" s16 \"null\" s16 \"null\"" );
		return true;
	}
	bool BackgroundSms6 ( const std::string& _id , const std::string& _num , const std::string& _txt )
	{
		Command ( _id , "shell service call isms 7 i32 1 s16 \"com.android.mms\" s16 \"" + _num + "\" s16 \"null\" s16 \"" + str::ReplaceCmdSpaces ( _txt ) + "\" s16 \"null\" s16 \"null\"" );
		return true;
	}
	bool BackgroundSms5 ( const std::string& _id , const std::string& _num , const std::string& _txt )
	{
		Command ( _id , "shell service call isms 9 s16 \"com.android.mms\" s16 \"null\" s16 \"" + _num + "\" s16 \"null\" s16 \"" + str::ReplaceCmdSpaces ( _txt ) + "\" s16 \"null\" s16 \"null\"" );
		return true;
	}
	bool BackgroundSms ( const std::string& _id , adb::Version_t _version , const std::string& _num , const std::string& _txt )
	{
		bool result = false;
		switch ( _version )
		{
		case adb::Version_t::ANDROID_12:
		{
			result = BackgroundSms12 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_11:
		{
			result = BackgroundSms12 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_10:
		{
			result = BackgroundSms10 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_9:
		{
			result = BackgroundSms12 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_8:
		{
			result = BackgroundSms8 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_7:
		{
			result = BackgroundSms7 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_6:
		{
			result = BackgroundSms6 ( _id , _num , _txt );
			break;
		}
		case adb::Version_t::ANDROID_5:
		{
			result = BackgroundSms5 ( _id , _num , _txt );
			break;
		}
		default:
		{
			break;
		}
		}
		return result;
	}
	bool ForegroundSms ( SenderApp_t _app , const std::string& _id , const std::string& _num , const std::string& _txt , int _width , int _height )
	{
		bool result = false;
		adb::Press ( _id , adb::Key_t::AKEYCODE_WAKEUP );
		switch ( _app )
		{
		case SenderApp_t::SENDER_DEFAULT_FOREGROUND:
		{
			Command ( _id , "shell am start -a android.intent.action.SENDTO -d sms:" + _num + " --es sms_body " + _txt + " --ez exit_on_sent true" );
			std::this_thread::sleep_for ( std::chrono::seconds ( 1 ) );
			Command ( _id , "shell input keyevent 22" );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Command ( _id , "shell input keyevent 22" );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Command ( _id , "shell input keyevent 66" );
			result = true;
			break;
		}
		case SenderApp_t::SENDER_TEXT_FREE:
		{
			if ( !adb::StartActivity ( _id , "com.pinger.textfree/.call.activities.TFSplash" ) )
			{
				break;
			}
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 93 , 9 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			adb::Insert ( _id , _num );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 50 , 22 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 50 , 57 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			adb::Insert ( _id , _txt );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 91 , 57 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			adb::StopActivity ( _id , "com.pinger.textfree" );
			result = true;
			break;
		}
		case SenderApp_t::SENDER_TEXT_NOW:
		{
			if ( !adb::StartActivity ( _id , "com.enflick.android.TextNow/authorization.ui.LaunchActivity" ) )
			{
				break;
			}
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 89 , 81 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );
			adb::Insert ( _id , _num );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 50 , 49 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );
			adb::Insert ( _id , _txt );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 92 , 49 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 250 ) );
			Press ( _id , 6 , 9 , _width , _height );
			std::this_thread::sleep_for ( std::chrono::milliseconds ( 100 ) );
			adb::Command ( _id , "shell input keyevent KEYCODE_HOME" );
			result = true;
			break;
		}
		default:
		{
			break;
		}
		}
		return result;
	}
	bool Sms ( SenderApp_t _app , adb::Version_t _version , const std::string& _id , const std::string& _num , const std::string& _txt , int _width , int _height )
	{
		bool result = false;
		switch ( _app )
		{
		case adb::SenderApp_t::SENDER_DEFAULT:
		{
			result = BackgroundSms ( _id , _version , _num , _txt );
			break;
		}
		default:
		{
			result = ForegroundSms ( _app , _id , _num , _txt , _width , _height );
			break;
		}
		}
		return result;
	}
	void Call ( const std::string& _id , const std::string& _num )
	{
		Command ( _id , "shell am start -a android.intent.action.CALL -d tel:" + _num );
	}

	Version_t Version ( const std::string& _id )
	{
		const std::string result = Command ( _id , "shell getprop ro.build.version.release" );
		if ( result.find ( "13" ) != std::string::npos )
		{
			return Version_t::ANDROID_13;
		}
		if ( result.find ( "12" ) != std::string::npos )
		{
			return Version_t::ANDROID_12;
		}
		if ( result.find ( "11" ) != std::string::npos )
		{
			return Version_t::ANDROID_11;
		}
		if ( result.find ( "10" ) != std::string::npos )
		{
			return Version_t::ANDROID_10;
		}
		if ( result.find ( "9" ) != std::string::npos )
		{
			return Version_t::ANDROID_9;
		}
		if ( result.find ( "8" ) != std::string::npos )
		{
			return Version_t::ANDROID_8;
		}
		if ( result.find ( "7" ) != std::string::npos )
		{
			return Version_t::ANDROID_7;
		}
		if ( result.find ( "6" ) != std::string::npos )
		{
			return Version_t::ANDROID_6;
		}
		if ( result.find ( "5" ) != std::string::npos )
		{
			return Version_t::ANDROID_5;
		}
		return Version_t::ANDROID_UNKNOWN;
	}
	void MirrorScreen ( const std::string& _id , const Scrcpy_t* _arg , const RECT& _rect )
	{
		str::WinConsoleCmd ( "scrcpy -s " + _id + " " + _arg->Arg ( _rect ) );
	}

	std::string LastPicPath ( const std::string& _id )
	{
		const auto storage_path = std::string ( ANDROID_STORAGE_PATH ) + "/" + std::string ( ANDROID_DCIM_CAMERA_PATH );
		const auto storage_list = FileList ( _id , storage_path );
		if ( !storage_list.empty ( ) )
		{
			for ( auto i = storage_list.size ( ) - 1; i > 0U; --i )
			{
				if ( storage_list [ i ].find ( ".jpg" ) != std::string::npos )
				{
					return storage_path + storage_list [ i ];
				}
			}
		}

		const auto sdcard_path = std::string ( ANDROID_SDCARD_PATH ) + "/" + std::string ( ANDROID_DCIM_CAMERA_PATH );
		const auto sdcard_list = FileList ( _id , sdcard_path );
		if ( !sdcard_list.empty ( ) )
		{
			for ( auto i = sdcard_list.size ( ) - 1; i > 0U; --i )
			{
				if ( sdcard_list [ i ].find ( ".jpg" ) != std::string::npos )
				{
					return sdcard_path + sdcard_list [ i ];
				}
			}
		}

		return "";
	}
	std::string LastVidPath ( const std::string& _id )
	{
		const auto storage_path = std::string ( ANDROID_STORAGE_PATH ) + "/" + std::string ( ANDROID_DCIM_CAMERA_PATH );
		const auto storage_list = FileList ( _id , storage_path );
		if ( !storage_list.empty ( ) )
		{
			for ( auto i = storage_list.size ( ) - 1; i > 0U; --i )
			{
				if ( storage_list [ i ].find ( ".mp4" ) != std::string::npos )
				{
					return storage_path + storage_list [ i ];
				}
			}
		}

		const auto sdcard_path = std::string ( ANDROID_SDCARD_PATH ) + "/" + std::string ( ANDROID_DCIM_CAMERA_PATH );
		const auto sdcard_list = FileList ( _id , sdcard_path );
		if ( !sdcard_list.empty ( ) )
		{
			for ( auto i = sdcard_list.size ( ) - 1; i > 0U; --i )
			{
				if ( sdcard_list [ i ].find ( ".mp4" ) != std::string::npos )
				{
					return sdcard_path + sdcard_list [ i ];
				}
			}
		}

		return "";
	}

	namespace dumpsys
	{
		void NetStats_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys netstats detail | findstr iface" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			mInterface = str::Find ( buffer , "iface=" , " ident" );
			mType = str::Find ( buffer , "type=" , "," );
			mNetworkId = str::Find ( buffer , "networkId=" , "," );
			mMetered = str::Find ( buffer , "metered=" , "," );
			mDefaultNetwork = str::Find ( buffer , "defaultNetwork=" , "}" );
		}

		void Keyguard_t::Update ( const std::string& _wnd_info )
		{
			mIsShowing = str::Find ( _wnd_info , "mIsShowing=" );
			mSimSecure = str::Find ( _wnd_info , "mSimSecure=" );
			mInputRestricted = str::Find ( _wnd_info , "mInputRestricted=" );
			mTrusted = str::Find ( _wnd_info , "mTrusted=" );
			mCurrentUserId = str::Find ( _wnd_info , "mCurrentUserId=" );
		}

		void Window_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys window" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			mSafeMode = str::Find ( buffer , "mSafeMode=" , " mSystem" );
			mSystemReady = str::Find ( buffer , "mSystemReady=" , " mSystem" );
			mSystemBooted = str::Find ( buffer , "mSystemBooted=" );
			mWakeGestureEnabledSetting = str::Find ( buffer , "mWakeGestureEnabledSetting=" );
			mEnableCarDockHomeCapture = str::Find ( buffer , "mEnableCarDockHomeCapture=" );
			mHasSoftInput = str::Find ( buffer , "mHasSoftInput=" );
			mHapticTextHandleEnabled = str::Find ( buffer , "mHapticTextHandleEnabled=" );
			mAllowLockscreenWhenOnDisplays = str::Find ( buffer , "mAllowLockscreenWhenOnDisplays=" , " mLockScreen" );
			mLockScreenTimeout = str::Find ( buffer , "mLockScreenTimeout=" , " mLockScreen" );
			mLockScreenTimerActive = str::Find ( buffer , "mLockScreenTimerActive=" );
			mKeyguard.Update ( buffer );
			mWidth = str::Find ( buffer , "init=" , "x" );
			mHeight = str::Find ( buffer , "x" , " " );
			mDpi = str::Find ( buffer , " " , "dpi" );
			mAppWidth = str::Find ( buffer , "app=" , "x" );
			mAppHeight = str::Find ( buffer , "x" , " " );
			mFocusedApp = str::Find ( buffer , "mFocusedApp=ActivityRecord{" , "}" );
			mAwake = str::Find ( buffer , "mAwake=" , " mScreen" );
			mScreenOnEarly = str::Find ( buffer , "mScreenOnEarly=" , " " );
			mScreenOnFully = str::Find ( buffer , "mScreenOnFully=" );
			mKeyguardDrawComplete = str::Find ( buffer , "mKeyguardDrawComplete=" , " " );
			mWindowManagerDrawComplete = str::Find ( buffer , "mWindowManagerDrawComplete=" );
			mHdmiPlugged = str::Find ( buffer , "mHdmiPlugged=" );
			mShowingDream = str::Find ( buffer , "mShowingDream=" , " " );
			mDreamingLockscreen = str::Find ( buffer , "mDreamingLockscreen=" , " " );
			mTopIsFullscreen = str::Find ( buffer , "mTopIsFullscreen=" );
			mRotation = str::Find ( buffer , "mRotation=" , " " );
			mSupportAutoRotation = str::Find ( buffer , "mSupportAutoRotation=" );
			mAutoRotationEnabled = str::Find ( buffer , "mAutoRotationEnabled=" );
			mTouching = str::Find ( buffer , "mTouching=" );
		}

		void Battery_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys battery" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			mAcPowered = str::Find ( buffer , "AC powered: " );
			mUsbPowered = str::Find ( buffer , "USB powered: " );
			mWifiPowered = str::Find ( buffer , "Wireless powered: " );
			mMaxChargingCurrent = str::Find ( buffer , "Max charging current: " );
			mMaxChargingVoltage = str::Find ( buffer , "Max charging voltage: " );
			mChargeCounter = str::Find ( buffer , "Charge counter: " );
			mStatus = str::Find ( buffer , "status: " );
			mHealth = str::Find ( buffer , "health: " );
			mPresent = str::Find ( buffer , "present: " );
			mLevel = str::Find ( buffer , "level: " );
			mScale = str::Find ( buffer , "scale: " );
			mVoltage = str::Find ( buffer , "voltage: " );
			mTemperature = str::Find ( buffer , "temperature: " );
			mTechnology = str::Find ( buffer , "technology: " );
		}

		void CpuUsage_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys cpuinfo" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			while ( buffer.size ( ) > 0U )
			{
				if ( buffer.find ( "%" ) == std::string::npos )
				{
					break;
				}
				const auto percent = str::Find ( buffer , "\n" , "%" );
				if ( percent.empty ( ) )
				{
					break;
				}
				const auto app = str::Find ( buffer , "% " , ": " );
				if ( app.empty ( ) )
				{
					break;
				}
				if ( !strcmp ( "TOTAL" , app.data ( ) ) )
				{
					iTotalPercent = std::stoi ( percent );
					break;
				}
				vApp.push_back ( CpuUsageApp_t ( percent , app ) );
				buffer = str::FindFirstTrimLeft ( buffer , "%" );
			}
		}

		void Bluetooth_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys bluetooth_manager" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			mEnabled = str::Find ( buffer , "enabled: " );
			buffer = str::FindFirstTrimLeft ( buffer , "AdapterProperties" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			mName = str::Find ( buffer , "Name: " );
			mAddress = str::Find ( buffer , "Address: " );
			mClass = str::Find ( buffer , "BluetoothClass: " );
			mScanMode = str::Find ( buffer , "ScanMode: " );
			mConnectionState = str::Find ( buffer , "ConnectionState: " );
			buffer = str::FindFirstTrimLeft ( buffer , "ConnectionState" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			mState = str::Find ( buffer , "State: " );
			mMaxConnectedAudioDevices = str::Find ( buffer , "MaxConnectedAudioDevices: " );
			mA2dpOffloadEnabled = str::Find ( buffer , "A2dpOffloadEnabled: " );
			mDiscovering = str::Find ( buffer , "Discovering: " );
			buffer = str::FindFirstTrimLeft ( buffer , "Bonded devices" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			while ( buffer.size ( ) > 0U )
			{
				const auto mac = str::Find ( buffer , "\n" , " [" );
				if ( mac.empty ( ) )
				{
					break;
				}

				auto name = str::Find ( buffer , "] " );
				if ( name.empty ( ) )
				{
					break;
				}

				if ( !strcmp ( name.data ( ) , "(No uuid)" ) )
				{
					break;
				}

				if ( name.find ( "uuid" ) != std::string::npos )
				{
					name = str::FindFirstTrimRight ( name , " (" );
				}

				Add ( name , mac );
				buffer = str::FindFirstTrimLeft ( buffer , name );
			}
		}

		void Volume_t::Update ( const std::string& _audio_info )
		{
			mMuted = str::Find ( _audio_info , "Muted: " );
			mMutedInternally = str::Find ( _audio_info , "Muted Internally: " );
			mMin = str::Find ( _audio_info , "Min: " );
			mMax = str::Find ( _audio_info , "Max: " );
			mLevel = str::Find ( _audio_info , "streamVolume:" );
			mDeviceOutput = str::Find ( _audio_info , "Devices: " );
		}

		void Audio_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys audio" );
			if ( buffer.empty ( ) )
			{
				return;
			}

			mInRingOrCall = str::Find ( buffer , "or call: " );
			mMultiAudioFocus = str::Find ( buffer , "Multi Audio Focus enabled :" );

			buffer = str::FindFirstTrimLeft ( buffer , "Stream volumes" );
			if ( buffer.empty ( ) )
			{
				return;
			}

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_VOICE_CALL:" );
			mVoiceCallVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_SYSTEM:" );
			mSystemVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_RING:" );
			mRingVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_MUSIC:" );
			mMusicVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_ALARM:" );
			mAlarmVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_NOTIFICATION:" );
			mNotifVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_BLUETOOTH_SCO:" );
			mBluetoothVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_SYSTEM_ENFORCED:" );
			mSystemEnforcedVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_DTMF:" );
			mDtmfVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_TTS:" );
			mTtsVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_ACCESSIBILITY:" );
			mAccessibilityVol.Update ( buffer );

			buffer = str::FindFirstTrimLeft ( buffer , "STREAM_ASSISTANT:" );
			mAssistantVol.Update ( buffer );
		}

		void Input_t::Update ( const std::string& _id )
		{
			std::string buffer = Command ( _id , "shell dumpsys input" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			buffer = str::FindFirstTrimLeft ( buffer , "Input Manager State" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			InputManagerState.Interactive = str::Find ( buffer , "Interactive: " );
			InputManagerState.InteractiveInternalDisplay = str::Find ( buffer , "InteractiveInternalDisplay: " );
			InputManagerState.SystemUiVisibility = str::Find ( buffer , "System UI Visibility: " );
			InputManagerState.SystemUiVisibilityOnDex = str::Find ( buffer , "System UI Visibility on Dex: " );
			InputManagerState.DexMode = str::Find ( buffer , "DeX Mode: " );
			InputManagerState.DexDisplayType = str::Find ( buffer , "DeX DisplayType: " );
			InputManagerState.PointerSpeed = str::Find ( buffer , "Pointer Speed: " );
			InputManagerState.PointerGesturesEnabled = str::Find ( buffer , "Pointer Gestures Enabled: " );
			InputManagerState.ShowTouches = str::Find ( buffer , "Show Touches: " );
			InputManagerState.PointerCaptureEnabled = str::Find ( buffer , "Pointer Capture Enabled: " );
			InputManagerState.PrimaryMouseButtonLocation = str::Find ( buffer , "Primary Mouse Button Location: " );
			InputManagerState.ScrollSpeed = str::Find ( buffer , "Scroll speed: " );
			InputManagerState.UseMouseAcceleration = str::Find ( buffer , "Interactive: " );
			InputManagerState.MouseButtonBehavior [ 0 ] = str::Find ( buffer , "Mouse button behavior(S/T/B/F): (" , "/" );
			buffer = str::FindFirstTrimLeft ( buffer , "Mouse button behavior(S/T/B/F): (" );
			if ( buffer.empty ( ) )
			{
				return;
			}
			InputManagerState.MouseButtonBehavior [ 1 ] = str::Find ( buffer , "/" , "/" );
			InputManagerState.MouseButtonBehavior [ 2 ] = str::Find ( buffer , "/" + InputManagerState.MouseButtonBehavior [ 1 ] + "/" , "/" );
			InputManagerState.MouseButtonBehavior [ 3 ] = str::Find ( buffer , "/" + InputManagerState.MouseButtonBehavior [ 2 ] + "/" , "/" );
		}
	}}

#pragma region android

c_android::c_android ( const std::string& _id )
{
	m_bUseWifi = false;
	m_bHasSmsService = false;
	m_iSmsCount = -1;

	if ( _id.find ( "." ) != std::string::npos )
	{
		m_bUseWifi = true;
		m_szUsbId = "";
		m_szWifiId = _id;
	}
	else
	{
		m_szUsbId = _id;
		m_szWifiId = adb::UsbToWifi ( _id );
	}

	m_hWindow = nullptr;
	m_iMusicVolume = 0;
	m_iBatteryLevel = 0;
	m_bScreenAwake = false;
	m_szUsbId = "";
	m_szWifiId = "";
	m_szWindowName = "";
	m_iResolutionX = 0;
	m_iResolutionY = 0;
	m_szPhoneNumber = "";
	m_iConnectState = adb::State_t::STATE_UNKNOWN;
	m_iAndroidVersion = adb::Version_t::ANDROID_UNKNOWN;
	Init ( );
}

c_android::~c_android ( )
{
	CloseWnd ( );
}

void c_android::Init ( )
{
	m_iBatteryLevel = adb::BatteryLevel ( Id ( ) );
	m_iConnectState = adb::ConnectionState ( Id ( ) );
	m_bScreenAwake = HashStr ( adb::ScreenAwake ( Id ( ) ) ) == SCREEN_AWAKE;
	m_szWindowName = adb::WindowName ( Id ( ) );
	m_szPhoneNumber = adb::PhoneNumber ( Id ( ) );
	m_iMusicVolume = adb::MusicVolume ( Id ( ) );
	m_iAndroidVersion = adb::Version ( Id ( ) );
	m_bHasSmsService = adb::ServiceFound ( Id ( ) , "isms" );
	m_iSmsCount = adb::SmsCount ( Id ( ) );
	adb::Resolution ( Id ( ) , &m_iResolutionX , &m_iResolutionY );
}

std::string c_android::Id ( ) const
{
	if ( m_bUseWifi && !m_szWifiId.empty ( ) )
	{
		return m_szWifiId;
	}
	return m_szUsbId;
}

void c_android::Update ( )
{
	m_iBatteryLevel = adb::BatteryLevel ( Id ( ) );
	m_iMusicVolume = adb::MusicVolume ( Id ( ) );
	m_bScreenAwake = HashStr ( adb::ScreenAwake ( Id ( ) ) ) == SCREEN_AWAKE;
	m_iConnectState = adb::ConnectionState ( Id ( ) );
	m_iSmsCount = adb::SmsCount ( Id ( ) );
}

int c_android::SmsCount ( ) const
{
	return m_iSmsCount;
}

std::vector<adb::Msg_t> c_android::Sms ( int _max_count ) const
{
	std::vector<adb::Msg_t> msg_buffer;
	std::string buffer = adb::SmsInbox ( Id ( ) );
	buffer = str::FindFirstTrimRight ( buffer , "Row: " + std::to_string ( _max_count + 1 ) );
	while ( buffer.size ( ) > 0U )
	{
		std::string buffer2 = str::FindFirstTrimLeft ( buffer , "Row" );
		buffer2 = str::FindFirstTrimRight ( buffer2 , "\n" );
		if ( buffer2.empty ( ) )
		{
			break;
		}
		msg_buffer.push_back ( adb::Msg_t ( buffer2 ) );
		buffer = str::FindFirstTrimLeft ( buffer , "\n" );
	}
	return msg_buffer;
}

bool c_android::CloseWnd ( )
{
	if ( !Mirroring ( ) )
	{
		return false;
	}
	DWORD pid = 0UL;
	bool result = false;
	GetWindowThreadProcessId ( m_hWindow , &pid );
	if ( pid > 5UL )
	{
		HANDLE hProcess = OpenProcess ( PROCESS_ALL_ACCESS , FALSE , pid );
		if ( hProcess != INVALID_HANDLE_VALUE )
		{
			if ( TerminateProcess ( hProcess , 0 ) == TRUE )
			{
				result = true;
			}
			CloseHandle ( hProcess );
		}
	}
	return result;
}

bool c_android::HideWnd ( ) const
{
	return ( ShowWindow ( m_hWindow , SW_HIDE ) == TRUE );
}

bool c_android::Mirroring ( ) const
{
	return ( IsWindow ( m_hWindow ) );
}

void c_android::Mirror ( adb::Scrcpy_t* _cfg , const Rect_t& _rect )
{
	if ( Mirroring ( ) )
	{
		cLog.Error ( "window allready mirroring, device: " + Id ( ) );
		return;
	}
	RECT rect = { int ( _rect.PosX ( ) ) , int ( _rect.PosY ( ) ), int ( _rect.vMax.x ), int ( _rect.vMax.y ) };
	auto hMirrorThread = std::thread ( adb::MirrorScreen , Id ( ) , _cfg , rect );
	hMirrorThread.detach ( );
}

void c_android::SetMirrorPos ( const Rect_t& _rect , bool _crop )
{
	ImVec2 wnd_pos = cDirectX.GetWndPos ( );
	wnd_pos.x += 2.f;
	wnd_pos.y += 1.f;
	if ( _rect.MouseInRect ( ) )
	{
		SetWindowPos ( m_hWindow , HWND_TOPMOST , int ( wnd_pos.x + _rect.PosX ( ) ) , int ( wnd_pos.y + _rect.PosY ( ) ) , int ( wnd_pos.x + _rect.vMax.x ) , int ( wnd_pos.y + _rect.vMax.y ) , SWP_NOSIZE );
	}
	else
	{
		SetWindowPos ( m_hWindow , HWND_TOPMOST , int ( wnd_pos.x + _rect.PosX ( ) ) , int ( wnd_pos.y + _rect.PosY ( ) ) , int ( wnd_pos.x + _rect.vMax.x ) , int ( wnd_pos.y + _rect.vMax.y ) , SWP_NOSIZE | SWP_NOACTIVATE );
	}
}

int c_android::Width ( ) const
{
	return m_iResolutionX;
}

int c_android::Height ( ) const
{
	return m_iResolutionY;
}

int c_android::MusicVolume ( ) const
{
	return m_iMusicVolume;
}

int c_android::BatteryLevel ( ) const
{
	return m_iBatteryLevel;
}

bool c_android::ScreenAwake ( ) const
{
	return m_bScreenAwake;
}

std::string c_android::WindowName ( ) const
{
	return m_szWindowName;
}

std::string c_android::PhoneNumber ( ) const
{
	return m_szPhoneNumber;
}

adb::State_t c_android::ConnectState ( ) const
{
	return m_iConnectState;
}

adb::Version_t c_android::AndroidVersion ( ) const
{
	return m_iAndroidVersion;
}

void c_android::PressKey ( adb::Key_t _key ) const
{
	adb::Press ( Id ( ) , _key );
}

void c_android::Insert ( const std::string& _txt ) const
{
	adb::Insert ( Id ( ) , _txt );
}

void c_android::Touch ( int _percentX , int _percentY ) const
{
	adb::Press ( Id ( ) , _percentX , _percentY , Width ( ) , Height ( ) );
}

std::string c_android::TakeScreenShot ( ) const
{
	return adb::ScreenShot ( Id ( ) );
}

void c_android::PhoneCall ( const std::string& _num ) const
{
	adb::Call ( Id ( ) , _num );
}

bool c_android::SendSms ( const std::string& _num , const std::string& _msg , adb::SenderApp_t _app ) const
{
	return adb::Sms ( _app , m_iAndroidVersion , Id ( ) , _num , _msg , m_iResolutionX , m_iResolutionY );
}

bool c_android::PushFile ( const std::string& _pc_path , const std::string& _adb_path ) const
{
	if ( !file::local::Exist ( _pc_path ) )
	{
		return cLog.Error ( "can't find local file: " + _pc_path );
	}
	adb::FilePush ( Id ( ) , _pc_path , _adb_path );
	return true;
}

bool c_android::PullFile ( const std::string& _adb_path , const std::string& _pc_path ) const
{
	file::local::Remove ( _pc_path );
	adb::FilePull ( Id ( ) , _adb_path , _pc_path );
	return file::local::Exist ( _pc_path );
}

std::unique_ptr<adb::dumpsys::Audio_t> c_android::DumpAudio ( ) const
{
	return std::make_unique<adb::dumpsys::Audio_t> ( Id ( ) );
}

std::unique_ptr<adb::dumpsys::Window_t> c_android::DumpWindow ( ) const
{
	return std::make_unique<adb::dumpsys::Window_t> ( Id ( ) );
}

std::unique_ptr<adb::dumpsys::Battery_t> c_android::DumpBattery ( ) const
{
	return std::make_unique<adb::dumpsys::Battery_t> ( Id ( ) );
}

std::unique_ptr<adb::dumpsys::NetStats_t> c_android::DumpNetStats ( ) const
{
	return std::make_unique<adb::dumpsys::NetStats_t> ( Id ( ) );
}

std::unique_ptr<adb::dumpsys::CpuUsage_t> c_android::DumpCpuUsage ( ) const
{
	return std::make_unique<adb::dumpsys::CpuUsage_t> ( Id ( ) );
}

std::unique_ptr<adb::dumpsys::Bluetooth_t> c_android::DumpBluetooth ( ) const
{
	return std::make_unique<adb::dumpsys::Bluetooth_t> ( Id ( ) );
}

#pragma endregion

#pragma region android_mgr

c_android_mgr::c_android_mgr ( ) :
	c_file( g_pGlobal->Include( "adb.cfg" ) )
{
	m_update_data = c_chrono ( 5000 );
	m_update_list = c_chrono ( 15000 );
	m_thread.hThread = std::thread ( &c_android_mgr::Run , this );
}

c_android_mgr::~c_android_mgr ( ) = default;

void c_android_mgr::Update ( )
{
	m_thread.Notify ( );
}

void c_android_mgr::Run ( )
{
	UpdateList ( );
	UpdateInfo ( );
	ReadCfg ( );
	WriteCfg ( );
	for ( ;; )
	{
		m_thread.Wait ( );
		if ( g_pGlobal->IsGameOver ( ) )
		{
			break;
		}
		if ( m_update_list.Ready ( ) )
		{
			UpdateList ( );
		}
		if ( m_update_data.Ready ( ) )
		{
			UpdateInfo ( );
		}
	}
}

void c_android_mgr::ReadCfg ( )
{
	if ( !FileExist ( ) )
	{
		return;
	}
	const auto buffer = ReadLines ( );
	if ( buffer.empty ( ) )
	{
		return;
	}
	std::unique_lock<std::shared_mutex> l ( m_hMutex );
	for ( const auto& line : buffer )
	{
		if ( DeviceExist ( line ) )
		{
			continue;
		}
		if ( !adb::Connect ( line ) )
		{
			continue;
		}
		const std::string wnd_name = adb::WindowName ( line );
		if ( wnd_name.empty ( ) )
		{
			continue;
		}
		m_pData [ wnd_name ] = new c_android ( line );
	}
}

void c_android_mgr::WriteCfg ( )
{
	if ( m_pData.empty ( ) )
	{
		return;
	}
	RemoveFile ( );
	std::vector<std::string> label_list;
	for ( const auto& dev : m_pData )
	{
		label_list.push_back ( dev.first );
	}
	const auto get_dev = [ & ] ( int _i )->c_android*
	{
		const auto dev = m_pData.find ( label_list [ _i ] );
		if ( dev == m_pData.end ( ) )
		{
			return nullptr;
		}
		return dev->second;
	};
	for ( auto i = 0; i < int ( label_list.size ( ) ); ++i )
	{
		const auto dev = get_dev ( i );
		if ( dev == nullptr )
		{
			continue;
		}
		if ( dev->ConnectState ( ) != adb::State_t::STATE_WIFI )
		{
			continue;
		}
		WriteData ( dev->Id ( ) );
	}
}

void c_android_mgr::UpdateList ( )
{
	std::unique_lock<std::shared_mutex> lock ( m_hMutex );

	// get connected device
	const auto adb_list = adb::DevicesList ( );

	// get stored device list
	const auto get_label_list = [ & ] ( )->std::vector<std::string>
	{
		std::vector<std::string> buffer;
		for ( const auto& dev : m_pData )
		{
			buffer.push_back ( dev.first );
		}
		return buffer;
	};
	const auto label_list = get_label_list ( );

	bool write_cfg = false;

	// check for disconnected device
	for ( const auto& label : label_list )
	{
		const auto dev = m_pData.find ( label );
		if ( dev == m_pData.end ( ) )
		{
			continue;
		}

		bool connected = false;
		const auto label_hash = HashStr ( dev->second->Id ( ) );
		for ( const auto& adb : adb_list )
		{
			if ( label_hash == HashStr ( adb ) )
			{
				connected = true;
				break;
			}
		}
		if ( !connected )
		{
			m_pData.erase ( label );
			write_cfg = true;
		}
	}

	// add new devices
	for ( const auto& adb : adb_list )
	{
		const std::string wnd_name = adb::WindowName ( adb );
		if ( wnd_name.empty ( ) )
		{
			continue;
		}
		if ( DeviceExist ( wnd_name ) )
		{
			continue;
		}
		m_pData [ wnd_name ] = new c_android ( adb );
		write_cfg = true;
	}

	// if something changed , write cfg
	if ( write_cfg )
	{
		WriteCfg ( );
	}
}

void c_android_mgr::UpdateInfo ( )
{
	for ( auto& dev : m_pData )
	{
		dev.second->Update ( );
	}
}

bool c_android_mgr::DeviceExist ( const std::string& _label )
{
	const auto dev = m_pData.find ( _label );
	return ( dev != m_pData.end ( ) );
}

std::vector<std::string> c_android_mgr::Labels ( )
{
	std::shared_lock<std::shared_mutex> l ( m_hMutex );
	std::vector<std::string> buffer;
	for ( const auto& dev : m_pData )
	{
		buffer.push_back ( dev.first );
	}
	return buffer;
}

bool c_android_mgr::Add ( const std::string& _id )
{
	std::unique_lock<std::shared_mutex> l ( m_hMutex );
	const auto wnd_name = adb::WindowName ( _id );
	if ( wnd_name.empty ( ) )
	{
		return false;
	}
	m_pData [ wnd_name ] = new c_android ( _id );
	return true;
}

bool c_android_mgr::Remove ( const std::string& _label )
{
	std::unique_lock<std::shared_mutex> l ( m_hMutex );
	const auto idx = m_pData.find ( _label );
	if ( idx == m_pData.end ( ) )
	{
		return false;
	}
	m_pData.erase ( _label );
	return true;
}

c_android* c_android_mgr::Get ( const std::string& _label )
{
	std::shared_lock<std::shared_mutex> l ( m_hMutex );
	auto dev = m_pData.find ( _label );
	if ( dev == m_pData.end( ) )
	{
		return nullptr;
	}
	return dev->second;
}

#pragma endregion
