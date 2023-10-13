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

void updateMumble(real_t z, real_t x, real_t y, real_t horiz_angle, real_t vert_angle, char name[128], char status[16]) {
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

	// Same as avatar but for the camera.
	lm->fCameraPosition[0] = lm->fAvatarPosition[0] = x; //this is an fps so camera should be same as avatar
	lm->fCameraPosition[1] = lm->fAvatarPosition[1] = y;
	lm->fCameraPosition[2] = lm->fAvatarPosition[2] = z;

	double dirY = -sin(vert_angle);
	float  magnitude = sqrt(1 + (dirY * dirY));

	lm->fCameraFront[0] = lm->fAvatarFront[0] = sin(horiz_angle) / magnitude;
	lm->fCameraFront[1] = lm->fAvatarFront[1] = dirY / magnitude;
	lm->fCameraFront[2] = lm->fAvatarFront[2] = cos(horiz_angle) / magnitude;

	// Axis - Character seems to be about 0.6 meters tall
	lm->fCameraTop[0] = lm->fAvatarTop[0] = 0;
	lm->fCameraTop[1] = lm->fAvatarTop[1] = 0.6;
	lm->fCameraTop[2] = lm->fAvatarTop[2] = 0;

	wchar_t convertedName[256] = {};
	charToWide(name, convertedName, 128);

	// Identifier which uniquely identifies a certain player in a context (e.g. the ingame name).
	wcsncpy(lm->identity, convertedName, 256);
	// Context should be equal for players which should be able to hear each other positional and
	// differ for those who shouldn't (e.g. it could contain the server+port and team)
	memcpy(lm->context, status, 16);
	lm->context_len = 16;
}
