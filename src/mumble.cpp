#include "main.hpp" // to use real_t struct for world position
#include <cmath> // to convert rad to unit vec
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h> /* For O_* constants */
#endif // _WIN32

struct LinkedMem {
#ifdef _WIN32
	UINT32	uiVersion;
	DWORD	uiTick;
#else
	uint32_t uiVersion;
	uint32_t uiTick;
#endif
	float	fAvatarPosition[3];
	float	fAvatarFront[3];
	float	fAvatarTop[3];
	wchar_t	name[256];
	float	fCameraPosition[3];
	float	fCameraFront[3];
	float	fCameraTop[3];
	wchar_t	identity[256];
#ifdef _WIN32
	UINT32	context_len;
#else
	uint32_t context_len;
#endif
	unsigned char context[256];
	wchar_t description[2048];
};

LinkedMem* lm = NULL;

void initMumble() {
#ifdef _WIN32
	HANDLE hMapObject = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (hMapObject == NULL) {
		return;
	}

	lm = (LinkedMem*)MapViewOfFile(hMapObject, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMem));
	if (lm == NULL) {
		CloseHandle(hMapObject);
		hMapObject = NULL;
		return;
	}
#else
	char memname[256];
	snprintf(memname, 256, "/MumbleLink.%d", getuid());

	int shmfd = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);

	if (shmfd < 0) {
		return;
	}

	lm = (LinkedMem*)(mmap(NULL, sizeof(struct LinkedMem), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0));

	if (lm == (void*)(-1)) {
		lm = NULL;
		return;
	}
#endif
}

void charToWide(char original[], wchar_t converted[], int length) {
	for (int i = 0; i < length; i++)
	{
	    converted[i] = (wchar_t)original[i];
	}
}

void updateMumble(real_t x, real_t y, real_t yaw, char name[128], char status[16]) {
	if (!lm)
		return;

	if (lm->uiVersion != 2) {
		wcsncpy(lm->name, L"Barony", 256);
		wcsncpy(lm->description, L"Barony v4.0.2 native link v1.0.0", 2048);
		lm->uiVersion = 2;
	}
	lm->uiTick++;

	// Left handed coordinate system.
	// X positive towards "right".
	// Y positive towards "up".
	// Z positive towards "front".
	//
	// 1 unit = 1 meter

	// Unit vector pointing out of the avatar's eyes aka "At"-vector.
	lm->fAvatarFront[0] = cos(yaw); // yaw is a radian
	lm->fAvatarFront[1] = sin(yaw);
	lm->fAvatarFront[2] = 0.0f;

	// Unit vector pointing out of the top of the avatar's head aka "Up"-vector (here Top points straight up).
	lm->fAvatarTop[0] = 0.0f;
	lm->fAvatarTop[1] = 1.0f;
	lm->fAvatarTop[2] = 0.0f;

	// Position of the avatar (here standing slightly off the origin)
	lm->fAvatarPosition[0] = y / 4.0f; // 8.0 is 1 block length, and a block seems about 2 meters
	lm->fAvatarPosition[1] = 0.0f;
	lm->fAvatarPosition[2] = x / 4.0f;

	// Same as avatar but for the camera.
	lm->fCameraPosition[0] = lm->fAvatarPosition[0]; //this is an fps so camera should be same as avatar
	lm->fCameraPosition[1] = lm->fAvatarPosition[1];
	lm->fCameraPosition[2] = lm->fAvatarPosition[2];

	lm->fCameraFront[0] = lm->fAvatarFront[0];
	lm->fCameraFront[1] = lm->fAvatarFront[1];
	lm->fCameraFront[2] = lm->fAvatarFront[2];

	lm->fCameraTop[0] = lm->fAvatarTop[0];
	lm->fCameraTop[1] = lm->fAvatarTop[1];
	lm->fCameraTop[2] = lm->fAvatarTop[2];

	wchar_t convertedName[256] = {};
	charToWide(name, convertedName, 128);

	// Identifier which uniquely identifies a certain player in a context (e.g. the ingame name).
	wcsncpy(lm->identity, convertedName, 256);
	// Context should be equal for players which should be able to hear each other positional and
	// differ for those who shouldn't (e.g. it could contain the server+port and team)
	memcpy(lm->context, status, 16);
	lm->context_len = 16;
}
