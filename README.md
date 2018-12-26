# borgwin
A wrapper for using Borg Backup on windows using Cygwin.

## Compiling Borgwin

Use the Visual Studio project located in the src directory. 
### Prerequisites
- boost: The project file expects an environment variable "boost" with the installation directory of boost.

## Installing Borg and Borgwin on windows using Cygwin

This document describes the installation process using cygwin. It is based on borg installation instructions, which can be found here:
https://borgbackup.readthedocs.io/en/stable/installation.html

### Installing Borg Backup into Cygwin:

First download Cygwin from here:
https://cygwin.com/install.html

Download and execute cygwin-x86_64.exe. The default installation folder is c:\cygwin64. It is suggested to stick with the default value.

From the default package selection, add the following:
```
python3 python3-devel python3-setuptools
binutils gcc-g++
libopenssl openssl-devel
git make openssh
```

Always choose the newest stable versions.

When done, open the Cygwin terminal and install:
```
easy_install-3.x pip
pip install virtualenv
```
Use the tab key to auto-complete, which version of easy_install has been installed.
Create the virtual environment and activate it:
```
virtualenv --python=python3 borg-env
source borg-env/bin/activate
# install Borg + Python dependencies into virtualenv
pip install borgbackup
# or alternatively (if you want FUSE support):
# pip install borgbackup[fuse]
pip install -U borgbackup  # or ... borgbackup[fuse]
```

### Install Borgwin
Not much to do here. Just call Borgwin from the windows command line. 
```
borgwin --help
```

### Usage

Here is for now a replication of the help text:
```
Usage: borgwin [options] -- [borg-arguments...]
Available options:
  --help                                produce help message
  --version                             display borgwin version number
  -c [ --cygwin ] arg (=c:\cygwin64\bin)
                                        cygwin installation directory. E.g.
                                        c:\cygwin64\bin
  -d [ --workdir ] arg                  directory from where borg is started.
  -p [ --passphrase ] arg               phassphrase for repository (if needed).
                                        This option is omitted, the user is
                                        prompted to enter the passphrase.
  -e [ --env ] arg (=borg-env)          Python virtual environment, in which
                                        borg is installed.
  --debug                               display debug information.
  -n [ --dont-run ]                     doesn't execute borg. Use with --debug
                                        to display the resulting bash command
                                        line.


borg-arguments... are arguments to be passed to borg and are separated from
borgwin's own options by '--'.
Use $pu(...) to identify paths, which need to be converted to cygwin paths.

Examples:
borgwin -- --version
   to displays borg version.
borgwin -p "123 4" -- list $pu("h:\borgtest repo")
borgwin -p "123 4" --debug -- list $pu("h:\borgtest repo")::"Run 1"
   is translated to:
   borg list "/cygdrive/h/borgtest repo::Run 1"
borgwin -p "123 4" --debug -- create $pu("h:\borgtest repo")::"Run 2" $pu("h:\borg test")
   is translated to:
   borg create "/cygdrive/h/borgtest repo::Run 2" "/cygdrive/h/borg test"```
