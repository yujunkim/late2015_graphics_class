#include <fstream>
#include "texture.h"
#include "lodepng.h"

unsigned char *loadTGA(const char* filepath, int &width, int &height) {

	std::ifstream fin(filepath, std::ios::binary);
	if (fin)
	{
		printf("[Texture Loading] : %s\n", filepath);
		unsigned char header[12];
		unsigned char info[6];
		// read header/info from file
		fin.read((char*)header, 12);
		fin.read((char*)info, 6);
		// translate header/info
		width = info[0] | (info[1] << 8);
		height = info[2] | (info[3] << 8);
		int depth =   info[4] / 8;
		printf("width: %d, height: %d\n", width, height);

		// read data
		unsigned char *data = new unsigned char[width*height*4];
		unsigned char *temp = new unsigned char[width*height*depth];
		fin.read((char*)temp, width*height*depth);
		for(int j=0; j<height; ++j)
		for(int i=0; i<width; ++i) {
			data[4*(j*width+i)  ] = temp[depth*(j*width+i)+2];
			data[4*(j*width+i)+1] = temp[depth*(j*width+i)+1];
			data[4*(j*width+i)+2] = temp[depth*(j*width+i)  ];
			data[4*(j*width+i)+3] = depth==4 ? temp[depth*(j*width+i)+3] : 255;
		}
		delete [] temp;	fin.close();

		return data;
	}
	else return NULL;
}

void initTGA(GLuint *tex, const char *name, int &width, int &height) {	
	unsigned char *data = loadTGA(name, width, height);	
	glDeleteTextures(1, tex);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); 
	if(data) delete [] data;
}

void initPNG(GLuint *tex, const char *name, int &width, int &height) {
	unsigned char* buffer;
	unsigned char* image;
	size_t buffersize, imagesize;
	LodePNG_Decoder decoder;

	LodePNG_Decoder_init(&decoder);

	LodePNG_loadFile(&buffer, &buffersize, name); /*load the image file with given filename*/
	if ( !buffer || buffersize <= 0 ){
		printf("Couldn't open file\n");
		return;
	}
	LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize); /*decode the png*/

	glDeleteTextures(1, tex);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, decoder.infoPng.width, 
		decoder.infoPng.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
		image);

	width = decoder.infoPng.width;
	height = decoder.infoPng.height;
	printf("width: %d height: %d\n", width, height);
	free(buffer);
	free(image);
}

void initTex() {

}