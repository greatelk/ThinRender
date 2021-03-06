/*
 *  thin render - Mobile render engine based on OpenGL ES 2.0
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin render.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Author:         Marc Fernandez Vanaclocha <marc.fernandez@i2cat.net>
 */

#include "../Platform.h"
#include "AndroidFileSystem.h"
#include <sys/stat.h>
#include <errno.h>

AndroidFileSystem::AndroidFileSystem(){
	init();
}

AndroidFileSystem::~AndroidFileSystem(){
	release();
}

int AndroidFileSystem::openFile(string filePath){
	if(getFileSize(filePath) != -1){
		return 1;//the file exist and is loaded
	}
	AAsset* asset = AAssetManager_open(assetManager, filePath.c_str(), AASSET_MODE_UNKNOWN);

	if(asset == NULL){
		logErr("COULD NOT OPEN ASSET");
		return -1;
	}

	const void* pData = AAsset_getBuffer(asset);
	off_t fileLength = AAsset_getLength(asset);

	FileStruct* fileStruct = new FileStruct(fileLength+1);
	memcpy(fileStruct->data, pData, fileLength * sizeof(char));

	fileStruct->data[fileLength] = '\0';
	fileMap[filePath] = fileStruct;

	AAsset_close(asset);
	return 0;
}

int AndroidFileSystem::openExternalFile(string filePath){
	if(getFileSize(filePath) != -1){
		return 1;//the file exist and is loaded
	}
	std::string path = dataPath+filePath;
	FILE* fp;
	fp = fopen(path.c_str(), "r");
	if(fp == 0){
		logErr("AndroidFileSystem::openExternalFile unable to open asset %s", path.c_str());
		fclose(fp);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	unsigned long fileSize = ftell(fp);
	rewind(fp);

	char* buffer = (char*) malloc(fileSize+1);
	unsigned long readResult = fread(buffer, sizeof(char), fileSize, fp);

	if(readResult != fileSize){
		logErr("something bad happens, read result != file size");
		return -1;
	}
	buffer[fileSize] = '\0';

	FileStruct* fileStruct = new FileStruct((int)fileSize+1);

	memcpy(fileStruct->data, buffer, fileSize+1);
	fileMap[filePath] = fileStruct;

	free(buffer);
	fclose(fp);

	return 0;

}
int AndroidFileSystem::writeFile(string filePath, const char* content){
	FILE *fp;
	int index;
	std::string path = dataPath+filePath;
	fp = fopen(path.c_str(),"w"); /* open for writing */
	fprintf(fp,"%s", content);
	fclose(fp); /* close the file before ending program */
	return 0;
}

void AndroidFileSystem::init(){
}

void AndroidFileSystem::setAssetManager(ANativeActivity* nativeActivity){
	this->assetManager = nativeActivity->assetManager;
	dataPath = "/sdcard/renderScenes/";
	struct stat sb;
	int32_t res = stat(dataPath.c_str(), &sb);
	if(0 == res && sb.st_mode & S_IFDIR){
		logInf("'files/' dir already in app's internal data storage.");
	}else if(ENOENT == errno){
		mkdir(dataPath.c_str(), 0770);
	}
}

int AndroidFileSystem::getFileDescriptor(string filePath, long* start, long* length){
	AAsset* asset = AAssetManager_open(assetManager, filePath.c_str(), AASSET_MODE_UNKNOWN);
	if(asset == NULL){
		logErr("COULD NOT OPEN ASSET");
		return -1;
	}
	int fd = AAsset_openFileDescriptor(asset, (off_t*) start, (off_t*) length);
	if(fd < 0){
		return -1;
	}
	AAsset_close(asset);
	return fd;
}
