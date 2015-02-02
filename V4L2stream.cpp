#include <string>
#include <string.h>
#include <iostream>
#include <stdint.h>
#include <unistd.h>

#include "inc/constants.h"
#include "inc/types.h"
#include "inc/classes.h"

using std::cout;
using std::endl;
using std::string;

V4L2stream::V4L2stream( void ): width( DEFAULT_WIDTH ), height( DEFAULT_HEIGHT ), device( DEFAULT_DEVICE ), numBufs( DEFAULT_NUMBUFS ) { }
V4L2stream::V4L2stream( int in_width, int in_height, string in_device, int in_numBufs ): width( in_width ), height( in_height ), device( in_device ), numBufs( in_numBufs ) { }

V4L2stream::~V4L2stream( void )
{
	off( );
	close( fd );
}

void V4L2stream::setWidth( int32_t in_width )
{ width = in_width; }
int32_t V4L2stream::getWidth( void )
{ return width; }

void V4L2stream::setHeight( int32_t in_height )
{ height = in_height; }
int32_t V4L2stream::getHeight( void )
{ return height; }

void V4L2stream::setDevice( string in_device )
{ device = in_device; }
string V4L2stream::getDevice( void )
{ return device; }

void V4L2stream::setBufs( int32_t in_numBufs )
{ numBufs = in_numBufs; }
int32_t V4L2stream::getBufs( void )
{ return numBufs; }

void V4L2stream::init( void )
{
	fd = open( device.c_str( ), O_RDWR );
	if( -1 == fd )
	{
		std::cerr << "failed to open " << device << endl;
		return;
	}

	if( -1 == xioctl( fd, VIDIOC_QUERYCAP, &device_caps) )
	{
		std::cerr << "error while querying caps" << endl;
		return;
	}

	format.type                  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width         = width;
	format.fmt.pix.height        = height;
	format.fmt.pix.pixelformat   = V4L2_PIX_FMT_H264;
	format.fmt.pix.field         = V4L2_FIELD_NONE;

	if( -1 == xioctl( fd, VIDIOC_S_FMT, &format ) )
	{
		std::cerr << "error while setting format" << endl;
		return;
	}

	ext_ctrls.count = 2;
	ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	ext_ctrls.controls = ( v4l2_ext_control* )malloc( 2 * sizeof( v4l2_ext_control ) );

	ext_ctrls.controls[ 0 ].id     = V4L2_CID_EXPOSURE_AUTO;
	ext_ctrls.controls[ 0 ].value  = V4L2_EXPOSURE_MANUAL;
	ext_ctrls.controls[ 1 ].id     = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
	ext_ctrls.controls[ 1 ].value  = 0;

	if( -1 == xioctl( fd, VIDIOC_S_EXT_CTRLS, &ext_ctrls ) )
	{
		std::cerr << "error while setting controls" << endl;
		return;
	}

	request_bufs.count                 = numBufs;
	request_bufs.type                  = format.type;
	request_bufs.memory                = V4L2_MEMORY_MMAP;

	if( -1 == xioctl( fd, VIDIOC_REQBUFS, &request_bufs ) )
	{
		std::cerr << "error while requesting buffers" << endl;
		return;
	}

	buffer.type = request_bufs.type;
	buffer.memory = request_bufs.memory;

	buf_array = ( array_buf* )malloc( sizeof( array_buf ) * request_bufs.count );

	for(int i = 0; i < request_bufs.count; ++i)
	{
		buffer.index = i;
		if( -1 == xioctl( fd, VIDIOC_QUERYBUF, &buffer ) )
		{
			std::cerr << "error while querying buffer" << endl;
			return;
		}

		buf_array[ i ].start = ( uint8_t* )mmap( NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset );

		if( MAP_FAILED == buf_array[ i ].start )
		{
			std::cerr << "error mapping buf_array " << endl;
			return;
		}

		if( -1 == xioctl( fd, VIDIOC_QBUF, &buffer ) )
		{
			std::cerr << "error while initial enqueuing" << endl;
			return;
		}
	}
}

void V4L2stream::on( void )
{
	if( -1 == xioctl( fd, VIDIOC_STREAMON, &buffer.type ) )
	{
		std::cerr << "error while turning stream on" << endl;
		return;
	}

	buffer.index = 0;
}

void V4L2stream::off( void )
{
	if( -1 == xioctl( fd, VIDIOC_STREAMOFF, &buffer.type ) )
	{
		std::cerr << "error while turning stream off" << endl;
		return;
	}
}

void V4L2stream::getFrame( int ( *ps_callback )( uint8_t*, uint32_t ) )
{
	
	if( -1 == xioctl( fd, VIDIOC_DQBUF, &buffer ) )
	{
		std::cerr << "error while retrieving frame" << endl;
		return;
	}

	if ( -1 == ps_callback( buf_array[ buffer.index ].start, buffer.length ) )
		cout << "frame processing callback failed" << endl;

	if( -1 == xioctl( fd, VIDIOC_QBUF, &buffer ) )
	{
		std::cerr << "error while releasing buffer" << endl;
		return;
	}

	++buffer.index; buffer.index %= request_bufs.count;

}

int32_t V4L2stream::xioctl( int32_t file_desc, int32_t request, void* argp )
{
	int32_t retVal;

	do retVal = ioctl( file_desc, request, argp );
	while( -1 == retVal && EINTR == errno );

	return retVal;
}