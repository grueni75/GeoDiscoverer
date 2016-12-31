# Geo Discoverer

Geo Discoverer is an navigation app for hikers and bikers. Its core is based on platform 
independent C++ code. It uses an hardware abstraction such that only a few files have to 
be adapted to run it on a different platform. It currently supports Linux and Android 
as a platform.   

## Installation

The source code is structured as follows:

| Directory               | Contents                                                            |
| ----------------------- | ------------------------------------------------------------------- |
| Source/General          | Platform independent souce code.                                    |
| Source/Platform/Feature | Implementation of platform specific features required by the core.  |
| Source/Platform/Target  | Ports of the app for different platforms (e.g., Linux, Android)     |
| Source/Cockpit          | Cockpit apps that display navigation information.                   |

### To build the Linux version:

1. Change to the Source/Platform/Target/Linux directory
2. Ensure the headers of the required libraries are installed 
   (see the LIBS line in the Makefile)
3. Call make
4. Start the app by calling ./GeoDiscoverer

### To build the Android version:

1. Install Android Studio
2. Open the project Source/Platform/Target/Android
3. Build either the mobile app (for phones and tablets) or the wear app (for watches)
4. Install the app to your device

## Usage

See the [online help](http://geo-discoverer.appspot.com) for more information. 

## Contributing

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :D

## Credits

See the [About tiddler](http://geo-discoverer.appspot.com/#About) in the online help.  

## License

Geo Discoverer is free software. The source code is licensed under the GNU General 
Public License Version 3. See http://www.gnu.org/licenses for more information. 

