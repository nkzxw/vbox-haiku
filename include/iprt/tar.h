#include <iprt/time.h>
/** A tar handle */
typedef R3PTRTYPE(struct RTTARINTERNAL *)        RTTAR;
/** Pointer to a RTTAR interface handle. */
typedef RTTAR                                   *PRTTAR;
/** Nil RTTAR interface handle. */
#define NIL_RTTAR                                ((RTTAR)0)

/** A tar file handle */
typedef R3PTRTYPE(struct RTTARFILEINTERNAL *)    RTTARFILE;
/** Pointer to a RTTARFILE interface handle. */
typedef RTTARFILE                               *PRTTARFILE;
/** Nil RTTARFILE interface handle. */
#define NIL_RTTARFILE                            ((RTTARFILE)0)

/**
 * Opens a Tar archive.
 *
 * Use the mask to specify the access type. In create mode the target file
 * have not to exists.
 *
 * @returns IPRT status code.
 *
 * @param   phTar          Where to store the RTTAR handle.
 * @param   pszTarname     The file name of the tar archive to open.
 * @param   fMode          Open flags, i.e a combination of the RTFILE_O_* defines.
 *                         The ACCESS, ACTION and DENY flags are mandatory!
 */
RTR3DECL(int) RTTarOpen(PRTTAR phTar, const char* pszTarname, uint32_t fMode);

/**
 * Close the Tar archive.
 *
 * @returns IPRT status code.
 *
 * @param   hTar           Handle to the RTTAR interface.
 */
RTR3DECL(int) RTTarClose(RTTAR hTar);

/**
 * Open a file in the Tar archive.
 *
 * @remark: Write mode means append mode only. It is not possible to make
 * changes to existing files.
 *
 * @remark: Currently it is not possible to open more than one file in write
 * mode. Although open more than one file in read only mode (even when one file
 * is opened in write mode) is always possible.
 *
 * @returns IPRT status code.
 *
 * @param   hTar           The hande of the tar archive.
 * @param   phFile         Where to store the handle to the opened file.
 * @param   pszFilename    Path to the file which is to be opened. (UTF-8)
 * @param   fOpen          Open flags, i.e a combination of the RTFILE_O_* defines.
 *                         The ACCESS, ACTION flags are mandatory! DENY flags
 *                         are currently not supported.
 */
RTR3DECL(int) RTTarFileOpen(RTTAR hTar, PRTTARFILE phFile, const char *pszFilename, uint32_t fOpen);

/**
 * Close the file opened by RTTarFileOpen.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          The file handle to close.
 */
RTR3DECL(int) RTTarFileClose(RTTARFILE hFile);

/**
 * Changes the read & write position in a file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   offSeek        Offset to seek.
 * @param   uMethod        Seek method, i.e. one of the RTFILE_SEEK_* defines.
 * @param   poffActual     Where to store the new file position.
 *                         NULL is allowed.
 */
RTR3DECL(int) RTTarFileSeek(RTTARFILE hFile, uint64_t uOffset,  unsigned uMethod, uint64_t *poffActual);

/**
 * Gets the current file position.
 *
 * @returns File offset.
 * @returns ~0ULL on failure.
 *
 * @param   hFile          Handle to the file.
 */
RTR3DECL(uint64_t) RTTarFileTell(RTTARFILE hFile);

/**
 * Read bytes from a file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   pvBuf          Where to put the bytes we read.
 * @param   cbToRead       How much to read.
 * @param   *pcbRead       How much we actually read .
 *                         If NULL an error will be returned for a partial read.
 */
RTR3DECL(int) RTTarFileRead(RTTARFILE hFile, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Read bytes from a file at a given offset.
 * This function may modify the file position.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   uOffset        Where to read.
 * @param   pvBuf          Where to put the bytes we read.
 * @param   cbToRead       How much to read.
 * @param   *pcbRead       How much we actually read .
 *                         If NULL an error will be returned for a partial read.
 */
RTR3DECL(int) RTTarFileReadAt(RTTARFILE hFile, uint64_t uOffset, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Write bytes to a file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   pvBuf          What to write.
 * @param   cbToWrite      How much to write.
 * @param   *pcbWritten    How much we actually wrote.
 *                         If NULL an error will be returned for a partial write.
 */
RTR3DECL(int) RTTarFileWrite(RTTARFILE hFile, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Write bytes to a file at a given offset.
 * This function may modify the file position.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   uOffset        Where to write.
 * @param   pvBuf          What to write.
 * @param   cbToWrite      How much to write.
 * @param   *pcbWritten    How much we actually wrote.
 *                         If NULL an error will be returned for a partial write.
 */
RTR3DECL(int) RTTarFileWriteAt(RTTARFILE hFile, uint64_t uOffset, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Query the size of the file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   pcbSize        Where to store the filesize.
 */
RTR3DECL(int) RTTarFileGetSize(RTTARFILE hFile, uint64_t *pcbSize);

/**
 * Set the size of the file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   cbSize         The new file size.
 */
RTR3DECL(int) RTTarFileSetSize(RTTARFILE hFile, uint64_t cbSize);

/**
 * Gets the mode flags of an open file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   pfMode         Where to store the file mode, see @ref grp_rt_fs for details.
 */
RTR3DECL(int) RTTarFileGetMode(RTTARFILE hFile, uint32_t *pfMode);

/**
 * Changes the mode flags of an open file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   fMode          The new file mode, see @ref grp_rt_fs for details.
 */
RTR3DECL(int) RTTarFileSetMode(RTTARFILE hFile, uint32_t fMode);

/**
 * Gets the modification timestamp of the file.
 *
 * @returns IPRT status code.
 *
 * @param   pFile           Handle to the file.
 * @param   pTime           Where to store the time.
 */
RTR3DECL(int) RTTarFileGetTime(RTTARFILE hFile, PRTTIMESPEC pTime);

/**
 * Sets the modification timestamp of the file.
 *
 * @returns IPRT status code.
 *
 * @param   pFile           Handle to the file.
 * @param   pTime           The time to store.
 */
RTR3DECL(int) RTTarFileSetTime(RTTARFILE hFile, PRTTIMESPEC pTime);

/**
 * Gets the owner and/or group of an open file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile           Handle to the file.
 * @param   pUid            Where to store the owner user id. NULL is ok.
 * @param   pGid            Where to store the group id. NULL is ok.
 */
RTR3DECL(int) RTTarFileGetOwner(RTTARFILE hFile, uint32_t *pUid, uint32_t *pGid);

/**
 * Changes the owner and/or group of an open file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile           Handle to the file.
 * @param   uid             The new file owner user id. Use -1 (or ~0) to leave this unchanged.
 * @param   gid             The new group id. Use -1 (or ~0) to leave this unchanged.
 */
RTR3DECL(int) RTTarFileSetOwner(RTTARFILE hFile, uint32_t uid, uint32_t gid);

/******************************************************************************
 *   Convenience Functions                                                    *
 ******************************************************************************/

 * @returns IPRT status code.
RTR3DECL(int) RTTarFileExists(const char *pszTarFile, const char *pszFile);
 * @returns IPRT status code.
 * @returns IPRT status code.
RTR3DECL(int) RTTarExtractFileToBuf(const char *pszTarFile, void **ppvBuf, size_t *pcbSize, const char *pszFile, PFNRTPROGRESS pfnProgressCallback, void *pvUser);
 * @returns IPRT status code.
 * @returns IPRT status code.
 * @returns IPRT status code.