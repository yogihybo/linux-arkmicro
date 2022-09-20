#ifndef EAPDEVICE_H
#define EAPDEVICE_H


#include "ECSDKFramework.h"

using namespace ECSDKFrameWork;

class EapDevice : public IECAccessDevice
{
public:
	EapDevice();
	virtual ~EapDevice();

	virtual int32_t open() override;


	virtual int32_t read(void *data, uint32_t length) override;


	virtual int32_t write(void *data, uint32_t length) override;


	virtual void close() override;

};


#endif