wget http://developer.amd.com/Downloads/AMD-APP-SDK-v2.6-lnx64.tgz
tar zxvf AMD-APP-SDK-v2.6-lnx64.tgz
chmod +x Install-AMD-APP.sh
./Install-AMD-APP.sh
tar zxvf icd-registration.tgz
sudo cp -r etc/OpenCL/* /etc/OpenCL/
rm -r etc

#echo """
#deb http://ppa.launchpad.net/fajran/opencl/ubuntu lucid main
#deb-src http://ppa.launchpad.net/fajran/opencl/ubuntu lucid main
#""" >> /etc/apt/sources.list
sudo apt-get install python-numpy libboost1.40-all-dev python-setuptools

wget http://pypi.python.org/packages/source/p/pyopencl/pyopencl-2011.2.tar.gz
tar xfz pyopencl-2011.2.tar.gz
cd pyopencl-2011.2/
python configure.py                            \
   --boost-inc-dir=/usr/include/boost          \
   --boost-lib-dir=/usr/lib                    \
   --boost-python-libname=boost_python-mt-py26 \
   --cl-inc-dir=/opt/AMDAPP/include/           \
   --cl-lib-dir=/opt/AMDAPP/lib/x86_64         \
   --cl-libname=OpenCL 
make
sudo make install
cd examples
python demo.py
