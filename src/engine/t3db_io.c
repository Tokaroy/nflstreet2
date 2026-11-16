#include <string.h>
#include "engine/t3db.h"
#include "engine/t3db_table.h"

typedef struct TDbIOFileHdr_801FC5FC {
    unsigned char mMagic[2];
    unsigned char mUnknown2;
    unsigned char mVersion;
    unsigned char mUnknown4;
    unsigned int mEnd;
    unsigned int mBase;
} TDbIOFileHdr_801FC5FC;

unsigned int lbl_802F9548[256] = { 0 };

extern int fn_80208204(void *pUnused, TDbStmt_80205754 *pStmt, TDbResult_8020AD1C *pResult);
extern TDbTable_t *_TDbTblFindTable(TDbDatabase_801FAA00 *pDb, int id);
extern int _TDbTblDestroy(TDbTable_t *pTable);
extern int _TDbTblRowGrow(TDbTable_t *pTable, int count);
int fn_801FAEDC(TDbTable_t *pTable, TDbIOStream_801FA8C8 *pStream, int skip);
int fn_801FB5C0(TDbTable_t *pTable, TDbIOStream_801FA8C8 *pStream, int mode);
int fn_801FB9D0(TDbExpr_t *pExpr, TDbIOStream_801FA8C8 *pStream, int release);

static inline unsigned short TDbTableFree_801FAAF8(TDbTable_t *pTable)
{
    return pTable->mCapacity - pTable->mRowCount;
}

static inline void TDbRowSetBit_801FAEDC(TDbTable_t *pTable, unsigned int row)
{
    unsigned char *pRow = pTable->mRows + pTable->mRowSize * row;
    unsigned int bit = pTable->mDeletedBit;

    pRow += bit >> 3;
    *pRow |= 1 << (7 - (bit & 7));
}

void fn_801FA870(void)
{
    unsigned short i;
    unsigned short j;
    int crc;

    for (i = 0; i <= 0xFF; i++) {
        crc = i << 24;
        for (j = 8; j != 0; j--) {
            crc = crc < 0 ? (crc << 1) ^ 0x04C11DB7 : crc << 1;
        }
        lbl_802F9548[i] = crc;
    }
}

int fn_801FA8C8(TDbIOStream_801FA8C8 *pStream, void *pData, unsigned int size)
{
    unsigned int done;
    int err;

    err = pStream->mDevice->mWrite(pStream->mHandle, pData, size, &done);
    if (err == 0) {
        if (size != done) {
            err = 0x24;
        } else {
            pStream->mOffset += size;
            pStream->mCrcA = fn_801FC950(pData, size, pStream->mCrcA);
            pStream->mCrcB = fn_801FC950(pData, size, pStream->mCrcB);
        }
    }
    return err;
}

int fn_801FA964(TDbIOStream_801FA8C8 *pStream, void *pData, unsigned int size)
{
    unsigned int done;
    int err;

    err = pStream->mDevice->mRead(pStream->mHandle, pData, size, &done);
    if (err == 0) {
        if (size != done) {
            err = 0x24;
        } else {
            pStream->mOffset += size;
            pStream->mCrcA = fn_801FC950(pData, size, pStream->mCrcA);
            pStream->mCrcB = fn_801FC950(pData, size, pStream->mCrcB);
        }
    }
    return err;
}

unsigned int fn_801FAA00(TDbDatabase_801FAA00 *pDb, unsigned int *pOut)
{
    TDbTable_t *pTable;
    TDbIndex_t *pIndex;
    TDbExpr_t *pFilter;
    unsigned int size;
    unsigned int pad;

    size = 0;
    for (pTable = pDb->mTables; pTable != 0; pTable = pTable->mNext) {
        if (pTable->mFlags & 2) {
            pOut[0] = pTable->mUnknown1C;
            pOut[1] = size;
            pOut += 2;
            size += 0x24;
            size += pTable->mColumnCount * sizeof(TDbColumn_t);
            size += pTable->mCapacity * pTable->mRowSize;
            for (pIndex = pTable->mIndexes; pIndex != 0; pIndex = pIndex->mNext) {
                if (pIndex->mDestroyed == 0) {
                    size += 8;
                    size += pIndex->mTree.mPartCount * 8;
                    pFilter = pIndex->mFilter;
                    if (pFilter != 0) {
                        size += sizeof(TDbExpr_t);
                        if (pFilter->mLeft.mType <= TDB_TYPE_BINARY) {
                            size += pFilter->mLeft.mSize;
                        }
                        if (pFilter->mRight.mType <= TDB_TYPE_BINARY) {
                            size += pFilter->mRight.mSize;
                        }
                    }
                }
            }
            if ((pTable->mFlags & 0x60) && pTable->mUnknown2C != 0) {
                pad = pTable->mUnknown2C & 7;
                size += pTable->mUnknown2C;
                if (pad) {
                    size += 8 - pad;
                }
            }
            size += 4;
        }
    }
    return size;
}

int fn_801FAAF8(TDbTable_t *pTable, TDbIOTableHdr_801FAAF8 *pHdr, int unused, TDbIOStream_801FA8C8 *pStream)
{
    TDbColumn_t columns[0xFE];
    TDbIndex_t *pIndex;
    unsigned short count;
    unsigned short remaining;
    int appended;
    int err;

    count = pHdr->mRowCount - pHdr->mUnknown14;
    appended = 0;
    if ((pTable->mFlags & 0x60) || (pHdr->mFlags & 0x60)) {
        return 0x31;
    }
    if (count != 0 && (pTable->mFlags & 8) && pTable->mCapacity == 0 && pTable->mUnknown40 != 0) {
        _TDbTblRowGrow(pTable, pTable->mUnknown40);
    }
    if (count > (unsigned short)(TDbTableFree_801FAAF8(pTable) + pTable->mUnknown46)) {
        return 0x13;
    }
    if (pTable->mColumnCount != pHdr->mColumnCount) {
        return 0x18;
    }
    err = fn_801FA964(pStream, columns, pTable->mColumnCount * sizeof(TDbColumn_t));
    if (err == 0 && memcmp(pTable->mColumns, columns, pTable->mColumnCount * sizeof(TDbColumn_t)) != 0) {
        err = 0x18;
    }
    if ((pHdr->mFlags & 0x18) == 0x18) {
        remaining = 0;
    } else {
        remaining = pHdr->mUnknown10;
    }
    if (pHdr->mRowCount != 0) {
        if (pTable->mRowCount == 0 && pHdr->mRowCount <= pTable->mCapacity) {
            appended = 1;
            pTable->mUnknown46 = pHdr->mUnknown14;
            pTable->mRowCount = pHdr->mRowCount;
            pTable->mUnknown48 = pHdr->mUnknown16;
            err = fn_801FA964(pStream, pTable->mRows, pTable->mRowSize * pHdr->mRowCount);
            count = pHdr->mRowCount;
            remaining -= pHdr->mRowCount;
        } else if (pHdr->mUnknown14 == 0 && pHdr->mRowCount <= pTable->mCapacity - pTable->mRowCount) {
            appended = 1;
            err = fn_801FA964(pStream, pTable->mRows + pTable->mRowSize * pTable->mRowCount,
                              pTable->mRowSize * pHdr->mRowCount);
            pTable->mRowCount += pHdr->mRowCount;
            count = pHdr->mRowCount;
            remaining -= pHdr->mRowCount;
        } else {
            while (count != 0 && err == 0) {
                if (count != 0) {
                    err = fn_801FAEDC(pTable, pStream, 0);
                }
                if (err == 0x16) {
                    err = 0;
                } else if (err == 0) {
                    count--;
                }
                remaining--;
            }
        }
    }
    if (remaining != 0) {
        if (pTable->mUnknown46 == 0 && remaining <= pTable->mCapacity - pTable->mRowCount) {
            err = fn_801FA964(pStream, pTable->mRows + pTable->mRowSize * pTable->mRowCount,
                              pTable->mRowSize * remaining);
        } else {
            while (remaining != 0 && err == 0) {
                err = fn_801FAEDC(pTable, pStream, 1);
                if (err == 0x17) {
                    err = 0;
                }
                remaining--;
            }
        }
    }
    if (appended && err == 0) {
        for (pIndex = pTable->mIndexes; pIndex != 0; pIndex = pIndex->mNext) {
            for (count = pTable->mRowCount - count; count < pTable->mRowCount; count++) {
                if (!TDbRowBit_801FAAF8(pTable, count)) {
                    err = _TDbIndexRowAdd(pIndex, count);
                    if (err != 0) {
                        return err;
                    }
                }
            }
            if (err != 0) {
                break;
            }
        }
    }
    if (err == 0) {
        while (pHdr->mIndexCount != 0) {
            err = fn_801FB5C0(pTable, pStream, 1);
            if (err != 0) {
                break;
            }
            pHdr->mIndexCount--;
        }
    }
    return err;
}

int fn_801FAEDC(TDbTable_t *pTable, TDbIOStream_801FA8C8 *pStream, int skip)
{
    unsigned char buffer[0x2000];
    unsigned char *pRow;
    TDbIndex_t *pIndex;
    unsigned short row;
    int reused;
    int err;

    err = 0;
    row = 0;
    reused = 0;
    if (!skip) {
        if (pTable->mRowCount < pTable->mCapacity) {
            row = pTable->mRowCount++;
        } else if (pTable->mUnknown46 != 0) {
            row = pTable->mUnknown48;
            reused = 1;
            pTable->mUnknown46--;
            pTable->mUnknown48 = *(unsigned short *)(pTable->mRows + pTable->mRowSize * row);
        } else {
            err = 0x13;
        }
    }
    if (err == 0) {
        if (!skip) {
            pRow = pTable->mRows + pTable->mRowSize * row;
        } else {
            pRow = buffer;
        }
        err = fn_801FA964(pStream, pRow, pTable->mRowSize);
        if (err == 0 && !skip && TDbRowBit_801FAAF8(pTable, row)) {
            err = 0x16;
        }
        if (!skip) {
            if (err == 0) {
                for (pIndex = pTable->mIndexes; pIndex != 0; pIndex = pIndex->mNext) {
                    err = _TDbIndexRowAdd(pIndex, row);
                    if (err != 0) {
                        break;
                    }
                }
                if (err != 0) {
                    for (pIndex = pTable->mIndexes; pIndex != 0; pIndex = pIndex->mNext) {
                        _TDbIndexRowDelete(pIndex, row);
                    }
                }
            }
            if (err != 0) {
                if (reused) {
                    memset(pRow, 0, pTable->mRowSize);
                    TDbRowSetBit_801FAEDC(pTable, row);
                    *(unsigned short *)(pTable->mRows + pTable->mRowSize * row) = pTable->mUnknown48;
                    pTable->mUnknown48 = row;
                    pTable->mUnknown46++;
                } else {
                    pTable->mRowCount--;
                }
            }
        }
    }
    return err;
}

int fn_801FB0F8(TDbDatabase_801FAA00 *pDb, TDbIOTableHdr_801FAAF8 *pHdr, int id, TDbIOStream_801FA8C8 *pStream)
{
    TDbTable_t *pTable;
    unsigned char pad[8];
    unsigned int i;
    int err;

    _TDbMemNotify(2, (int)pDb->mUnknown8, id);
    err = TDbExtMemAlloc(pDb->mHeap, sizeof(TDbTable_t) + (pHdr->mColumnCount - 1) * sizeof(TDbColumn_t),
                      &pTable);
    if (err == 0) {
        pTable->mRowSize = pHdr->mRowSize;
        pTable->mCapacity = pHdr->mUnknown10;
        pTable->mUnknown40 = pHdr->mUnknown10;
        pTable->mRowCount = pHdr->mRowCount;
        pTable->mColumnCount = pHdr->mColumnCount;
        pTable->mDeletedBit = pHdr->mDeletedBit;
        pTable->mDb = pDb;
        pTable->mUnknown1C = id;
        pTable->mFlags = pHdr->mFlags;
        pTable->mFlags &= ~1;
        pTable->mRowCount = pHdr->mRowCount;
        pTable->mUnknown46 = pHdr->mUnknown14;
        pTable->mUnknown48 = pHdr->mUnknown16;
        pTable->mUnknown2C = pHdr->mDataSize;
        pTable->mUnknownC = pTable->mUnknown10 = 0;
        pTable->mIndexes = 0;
        pTable->mRows = 0;
        if ((pTable->mFlags & 0x18) == 0x18) {
            pTable->mCapacity = 0;
        }
        err = fn_801FA964(pStream, pTable->mColumns, pTable->mColumnCount * sizeof(TDbColumn_t));
        if (err == 0 && pTable->mCapacity != 0) {
            err = TDbExtMemAlloc(pDb->mHeap, pTable->mRowSize * pTable->mCapacity, &pTable->mRows);
            if (err == 0) {
                err = fn_801FA964(pStream, pTable->mRows, pTable->mRowSize * pTable->mCapacity);
                if (err != 0) {
                    TDbExtMemFree(pDb->mHeap, pTable->mRows, pTable->mRowSize * pTable->mCapacity);
                }
            }
        }
        if (err == 0) {
            if ((pTable->mFlags & 0x60) && err == 0 && pTable->mUnknown2C != 0) {
                err = TDbExtMemAlloc(pDb->mHeap, pTable->mUnknown2C, &pTable->mUnknown18);
                if (err == 0) {
                    err = fn_801FA964(pStream, pTable->mUnknown18, pTable->mUnknown2C);
                    if (err == 0 && (pTable->mUnknown2C & 7)) {
                        err = pStream->mDevice->mSeek(pStream->mHandle, 8 - (pTable->mUnknown2C & 7));
                        memset(pad, 0, sizeof(pad));
                        pStream->mOffset += 8 - (pTable->mUnknown2C & 7);
                        pStream->mCrcA = fn_801FC950(pad, 8 - (pTable->mUnknown2C & 7), pStream->mCrcA);
                        pStream->mCrcB = fn_801FC950(pad, 8 - (pTable->mUnknown2C & 7), pStream->mCrcB);
                    }
                    if (err != 0) {
                        TDbExtMemFree(pDb->mHeap, pTable->mUnknown18, pTable->mUnknown2C);
                    }
                }
            }
        }
        if (err != 0) {
            TDbExtMemFree(pDb->mHeap, pTable, sizeof(TDbTable_t) + (pHdr->mColumnCount - 1) * sizeof(TDbColumn_t));
        }
    }
    if (err == 0) {
        while (pHdr->mIndexCount != 0) {
            err = fn_801FB5C0(pTable, pStream, 0);
            if (err != 0) {
                break;
            }
            pHdr->mIndexCount--;
        }
        if (err != 0) {
            while (pTable->mIndexes != 0) {
                _TDbIndexDestroy(pTable->mIndexes);
            }
            if ((pTable->mFlags & 0x60) && pTable->mUnknown2C != 0) {
                TDbExtMemFree(pDb->mHeap, pTable->mUnknown18, pTable->mUnknown2C);
            }
            TDbExtMemFree(pDb->mHeap, pTable->mRows, pTable->mRowSize * pTable->mCapacity);
            TDbExtMemFree(pDb->mHeap, pTable,
                        sizeof(TDbTable_t) + (pHdr->mColumnCount - 1) * sizeof(TDbColumn_t));
        }
        if (err == 0) {
            if (pDb->mTables == 0) {
                pDb->mTables = pTable;
            } else {
                pTable->mNext = pDb->mTables;
                while (pTable->mNext->mNext != 0) {
                    pTable->mNext = pTable->mNext->mNext;
                }
                pTable->mNext->mNext = pTable;
            }
            pTable->mNext = 0;
            for (i = 0; i < 4; i++) {
                pTable->mCallbacks[i] = 0;
            }
        }
    }
    _TDbMemNotifyEnd();
    return err;
}

int fn_801FB5C0(TDbTable_t *pTable, TDbIOStream_801FA8C8 *pStream, int skip)
{
    TDbExpr_t expr;
    TDbIOIndexHdr_801FBC2C hdr;
    TDbIndex_t *pIndex;
    TDbIOIndexPart_801FBC2C part;
    unsigned char i;
    unsigned short row;
    int err;

    pIndex = 0;
    err = fn_801FA964(pStream, &hdr, sizeof(hdr));
    if (err == 0) {
        if (!skip) {
            if (hdr.mPartCount <= 2) {
                err = __TDbMemPoolAlloc(&lbl_803E96A8, &pIndex);
                pIndex->mFlags = 4;
            } else {
                err = TDbExtMemAlloc(pTable->mDb->mHeap,
                                  sizeof(TDbIndex_t) + (hdr.mPartCount - 1) * sizeof(TDbIdxPart_t),
                                  &pIndex);
                pIndex->mFlags = 0;
            }
        }
        if (err == 0) {
            if (!skip) {
                pIndex->mLocked = 0;
                pIndex->mCount = 0;
                pIndex->mDestroyed = 0;
                pIndex->mFilter = 0;
                pIndex->mNext = 0;
                pIndex->mTable = pTable;
                pIndex->mId = hdr.mId;
                pIndex->mTree.mPartCount = hdr.mPartCount;
                pIndex->mTree.mAllowDuplicates = hdr.mAllowDuplicates;
                pIndex->mTree.mRoot = 0;
            }
            for (i = 0; i < hdr.mPartCount; i++) {
                err = fn_801FA964(pStream, &part, sizeof(part));
                if (err == 0) {
                    if (!skip) {
                        pIndex->mTree.mParts[i].mAscending = part.mAscending;
                        pIndex->mTree.mParts[i].mColumn = part.mColumn;
                        if (pTable->mColumns[part.mColumn].mType <= TDB_TYPE_BINARY) {
                            pIndex->mFlags |= 1;
                        }
                        pIndex->mTree.mParts[i].mTable = pTable;
                    }
                } else {
                    break;
                }
            }
            if (hdr.mHasFilter == 1 && err == 0) {
                err = fn_801FB9D0(&expr, pStream, skip);
                if (err == 0 && !skip) {
                    err = _TDbExprPrepare(&expr, &pIndex->mFilter, &pTable, 1, 1);
                }
            }
            if (!skip) {
                if (err == 0) {
                    err = _TDbMemPoolInit(&pIndex->mPool, pTable->mDb->mHeap,
                                      sizeof(TDbIdxNode_t) + (hdr.mPartCount - 1) * sizeof(unsigned int),
                                      pTable->mCapacity + 5, 100);
                }
                if (err != 0) {
                    if (pIndex->mFlags & 4) {
                        __TDbMemPoolFree(&lbl_803E96A8, pIndex);
                    } else {
                        TDbExtMemFree(pTable->mDb->mHeap, pIndex,
                                    sizeof(TDbIndex_t) + (hdr.mPartCount - 1) * sizeof(TDbIdxPart_t));
                    }
                    pIndex = 0;
                }
            }
        }
    }
    if (!skip && err == 0) {
        for (row = 0; row < pTable->mRowCount; row++) {
            if (!TDbRowBit_801FAAF8(pTable, row)) {
                err = _TDbIndexRowAdd(pIndex, row);
                if (err != 0) {
                    break;
                }
            }
        }
        if (err != 0) {
            _TDbMemPoolDeinit(&pIndex->mPool);
            if (pIndex->mFlags & 4) {
                __TDbMemPoolFree(&lbl_803E96A8, pIndex);
            } else {
                TDbExtMemFree(pTable->mDb->mHeap, pIndex,
                            sizeof(TDbIndex_t) + (hdr.mPartCount - 1) * sizeof(TDbIdxPart_t));
            }
        }
        if (err == 0) {
            if (pTable->mIndexes == 0) {
                pTable->mIndexes = pIndex;
            } else {
                pIndex->mNext = pTable->mIndexes;
                while (pIndex->mNext->mNext != 0) {
                    pIndex->mNext = pIndex->mNext->mNext;
                }
                pIndex->mNext->mNext = pIndex;
            }
            pIndex->mNext = 0;
        }
    }
    return err;
}

int fn_801FB9D0(TDbExpr_t *pExpr, TDbIOStream_801FA8C8 *pStream, int release)
{
    int err;

    err = fn_801FA964(pStream, pExpr, sizeof(TDbExpr_t));
    if (err == 0) {
        if (pExpr->mLeft.mType <= TDB_TYPE_BINARY) {
            err = TDbExtMemAlloc(lbl_803EBE8C, pExpr->mLeft.mSize, &pExpr->mLeft.mValue.p);
            if (err == 0) {
                err = fn_801FA964(pStream, pExpr->mLeft.mValue.p, pExpr->mLeft.mSize);
            }
            if (release) {
                TDbExtMemFree(lbl_803EBE8C, pExpr->mLeft.mValue.p, pExpr->mRight.mSize);
            }
        }
        if (pExpr->mRight.mType <= TDB_TYPE_BINARY && err == 0) {
            err = TDbExtMemAlloc(lbl_803EBE8C, pExpr->mRight.mSize, &pExpr->mRight.mValue.p);
            if (err == 0) {
                err = fn_801FA964(pStream, pExpr->mRight.mValue.p, pExpr->mRight.mSize);
            }
            if (release) {
                TDbExtMemFree(lbl_803EBE8C, pExpr->mRight.mValue.p, pExpr->mRight.mSize);
            }
        }
    }
    return err;
}

int fn_801FBAC4(TDbTable_t *pTable, TDbExpr_t *pExpr, TDbIOStream_801FA8C8 *pStream)
{
    TDbExpr_t expr;
    int err;

    expr = *pExpr;
    if (expr.mLeft.mType == TDB_TYPE_COLUMN) {
        expr.mLeft.mValue.l = (unsigned long long)pTable->mUnknown1C << 32 |
                              pTable->mColumns[(unsigned char)expr.mLeft.mValue.l].mUnknown8;
    }
    if (expr.mRight.mType == TDB_TYPE_COLUMN) {
        expr.mRight.mValue.l = (unsigned long long)pTable->mUnknown1C << 32 |
                               pTable->mColumns[(unsigned char)expr.mRight.mValue.l].mUnknown8;
    }
    err = fn_801FA8C8(pStream, &expr, sizeof(expr));
    if (pExpr->mLeft.mType <= TDB_TYPE_BINARY && err == 0) {
        err = fn_801FA8C8(pStream, pExpr->mLeft.mValue.p, pExpr->mLeft.mSize);
    }
    if (pExpr->mRight.mType <= TDB_TYPE_BINARY && err == 0) {
        err = fn_801FA8C8(pStream, pExpr->mRight.mValue.p, pExpr->mRight.mSize);
    }
    return err;
}

int fn_801FBC2C(TDbIndex_t *pIndex, TDbIOStream_801FA8C8 *pStream)
{
    TDbIOIndexHdr_801FBC2C hdr;
    TDbIOIndexPart_801FBC2C part;
    unsigned char count;
    unsigned char i;
    int err;

    memset(&hdr, 0, sizeof(hdr));
    hdr.mId = pIndex->mId;
    count = pIndex->mTree.mPartCount;
    hdr.mAllowDuplicates = pIndex->mTree.mAllowDuplicates;
    hdr.mPartCount = count;
    hdr.mHasFilter = pIndex->mFilter != 0;
    err = fn_801FA8C8(pStream, &hdr, sizeof(hdr));
    if (err == 0) {
        for (i = 0; i < count; i++) {
            part.mColumn = pIndex->mTree.mParts[i].mColumn;
            part.mAscending = pIndex->mTree.mParts[i].mAscending;
            err = fn_801FA8C8(pStream, &part, sizeof(part));
            if (err != 0) {
                break;
            }
        }
    }
    if (hdr.mHasFilter && err == 0) {
        err = fn_801FBAC4(pIndex->mTable, pIndex->mFilter, pStream);
    }
    return err;
}

int fn_801FBD2C(TDbTable_t *pTable, TDbIOStream_801FA8C8 *pStream)
{
    TDbIOTableHdr_801FAAF8 hdr;
    unsigned int done;
    unsigned char pad[8];
    TDbIndex_t *pIndex;
    int rows;
    int err;

    memset(&hdr, 0, sizeof(hdr));
    hdr.mFlags = pTable->mFlags;
    hdr.mRowSize = pTable->mRowSize;
    hdr.mDeletedBit = pTable->mDeletedBit;
    hdr.mDataSize = pTable->mUnknown2C;
    if ((hdr.mFlags & 8) && pTable->mCapacity == 0) {
        hdr.mFlags |= 0x10;
        hdr.mUnknown10 = pTable->mUnknown40;
        rows = 0;
    } else {
        hdr.mFlags &= ~0x10;
        hdr.mUnknown10 = pTable->mCapacity;
        rows = 1;
    }
    hdr.mRowCount = pTable->mRowCount;
    hdr.mUnknown14 = pTable->mUnknown46;
    hdr.mUnknown16 = pTable->mUnknown48;
    hdr.mColumnCount = pTable->mColumnCount;
    hdr.mIndexCount = 0;
    for (pIndex = pTable->mIndexes; pIndex != 0; pIndex = pIndex->mNext) {
        if (pIndex->mDestroyed == 0) {
            hdr.mIndexCount++;
        }
    }
    pStream->mCrcB = -1;
    err = fn_801FA8C8(pStream, &hdr, sizeof(hdr));
    if (err == 0) {
        err = pStream->mDevice->mWrite(pStream->mHandle, &pStream->mCrcB, 4, &done);
        if (err == 0) {
            if (done != 4) {
                err = 0x24;
            } else {
                pStream->mOffset += 4;
            }
        }
    }
    pStream->mCrcB = -1;
    if (err == 0) {
        err = fn_801FA8C8(pStream, pTable->mColumns, pTable->mColumnCount * sizeof(TDbColumn_t));
    }
    if (rows == 1 && err == 0 && pTable->mCapacity != 0) {
        err = fn_801FA8C8(pStream, pTable->mRows, pTable->mRowSize * pTable->mCapacity);
    }
    if (err == 0) {
        if ((pTable->mFlags & 0x60) && pTable->mUnknown2C != 0) {
            err = fn_801FA8C8(pStream, pTable->mUnknown18, pTable->mUnknown2C);
            if (err == 0 && (pTable->mUnknown2C & 7)) {
                memset(pad, 0, sizeof(pad));
                err = fn_801FA8C8(pStream, pad, 8 - (pTable->mUnknown2C & 7));
            }
        }
        if (err == 0) {
            for (pIndex = pTable->mIndexes; pIndex != 0; pIndex = pIndex->mNext) {
                if (pIndex->mDestroyed == 0) {
                    err = fn_801FBC2C(pIndex, pStream);
                    if (err != 0) {
                        break;
                    }
                }
            }
            if (err == 0) {
                err = pStream->mDevice->mWrite(pStream->mHandle, &pStream->mCrcB, 4, &done);
                if (err == 0) {
                    if (done != 4) {
                        err = 0x24;
                    } else {
                        pStream->mOffset += 4;
                    }
                }
            }
        }
    }
    return err;
}

int fn_801FBFEC(TDbDatabase_801FAA00 *pDb, int *pIds, void *pName, unsigned int mode, TDbIODevice_t *pDevice)
{
    TDbIOFileHdr_801FC5FC hdr;
    TDbIOTableHdr_801FAAF8 tableHdr;
    TDbIOStream_801FA8C8 stream;
    TDbTable_t *pTable;
    unsigned int *pDir;
    unsigned char *pLoad;
    unsigned int count;
    unsigned int n;
    unsigned int countCrc;
    unsigned int dirCrc;
    unsigned int tableCrc;
    unsigned int done;
    unsigned int base;
    unsigned int i;
    int id;
    int err;

    err = 0;
    pDir = 0;
    pLoad = 0;
    _TDbMemNotify(1, (int)pDb->mUnknown8, -1);
    if (mode == 0 && pIds == 0) {
        while (pDb->mTables != 0) {
            _TDbTblDestroy(pDb->mTables);
        }
    }
    stream.mDevice = pDevice;
    stream.mUnknownC = 1;
    stream.mOffset = 0;
    stream.mCrcA = -1;
    err = pDevice->mOpen(stream.mHandle, 0, pName, 1);
    if (err == 0) {
        err = fn_801FA964(&stream, &hdr, sizeof(hdr));
        if (err == 0) {
            if ((*(unsigned int *)&hdr & 0xFFFF0000) != 0x44420000) {
                err = 0x2C;
            } else if ((*(unsigned int *)&hdr & 0xFFFF) != 8 || hdr.mUnknown4 != 1) {
                err = 0x25;
            }
            stream.mEnd = hdr.mEnd;
            stream.mSize = hdr.mEnd - hdr.mBase;
        }
        count = 0;
        if (err == 0) {
            err = fn_801FA964(&stream, &n, 4);
            count = n;
            if (err == 0) {
                err = pDevice->mRead(stream.mHandle, &countCrc, 4, &done);
                if (done != 4) {
                    err = 0x24;
                } else {
                    stream.mOffset += 4;
                    if (countCrc != stream.mCrcA) {
                        err = 0x2B;
                    }
                    stream.mCrcA = -1;
                }
            }
        }
        if (err == 0) {
            err = TDbExtMemAllocTemp(lbl_803EBE8C, count * 8 + (count + 7) / 8, &pDir);
            if (err == 0) {
                err = fn_801FA964(&stream, pDir, count * 8);
                if (err == 0) {
                    err = pDevice->mRead(stream.mHandle, &dirCrc, 4, &done);
                    if (done != 4) {
                        err = 0x24;
                    } else {
                        stream.mOffset += 4;
                        if (dirCrc != stream.mCrcA) {
                            err = 0x2B;
                        }
                        stream.mCrcA = -1;
                    }
                    if (err == 0) {
                        pLoad = (unsigned char *)pDir + count * 8;
                        if (pIds == 0) {
                            memset(pLoad, 0xFF, (count + 7) / 8);
                        } else {
                            memset(pLoad, 0, (count + 7) / 8);
                            for (; *pIds != -1; pIds++) {
                                err = 7;
                                for (i = 0; i < count; i++) {
                                    if (pDir[i * 2] == *pIds) {
                                        pLoad[i >> 3] |= 1 << (i & 7);
                                        err = 0;
                                        break;
                                    }
                                }
                                if (err != 0) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        base = stream.mOffset;
        for (i = 0; err == 0 && i < count; i++) {
            if ((pLoad[i >> 3] >> (i & 7)) & 1) {
                if (pDir[i * 2 + 1] > stream.mOffset - base) {
                    pDevice->mSeek(stream.mHandle, pDir[i * 2 + 1] - (stream.mOffset - base));
                    stream.mOffset = pDir[i * 2 + 1] + base;
                }
                id = pDir[i * 2];
                stream.mCrcB = -1;
                err = fn_801FA964(&stream, &tableHdr, sizeof(tableHdr));
                if (err == 0) {
                    err = pDevice->mRead(stream.mHandle, &tableCrc, 4, &done);
                    if (done != 4) {
                        err = 0x24;
                    } else {
                        stream.mOffset += 4;
                        if (tableCrc != stream.mCrcB) {
                            err = 0x2B;
                        }
                        stream.mCrcB = -1;
                    }
                }
                if (err == 0) {
                    pTable = _TDbTblFindTable(pDb, id);
                    switch (mode) {
                    case 0:
                        if (pTable != 0) {
                            _TDbTblDestroy(pTable);
                            pTable = 0;
                        }
                    case 3:
                        if (pTable != 0) {
                            TDbStmt_80205754 stmt;
                            TDbResult_8020AD1C result;

                            stmt.mTableCount = 1;
                            stmt.mWhere = 0;
                            stmt.mTables[0] = pTable;
                            stmt.mTableExprs[0] = 0;
                            fn_80208204(pTable->mDb->mUnknown8, &stmt, &result);
                            err = fn_801FAAF8(pTable, &tableHdr, id, &stream);
                        } else {
                            err = fn_801FB0F8(pDb, &tableHdr, id, &stream);
                        }
                        break;
                    case 1:
                        if (pTable != 0) {
                            err = 6;
                        } else {
                            err = fn_801FB0F8(pDb, &tableHdr, id, &stream);
                        }
                        break;
                    case 2:
                        if (pTable != 0) {
                            err = fn_801FAAF8(pTable, &tableHdr, id, &stream);
                        } else {
                            err = fn_801FB0F8(pDb, &tableHdr, id, &stream);
                        }
                        break;
                    default:
                        err = 3;
                        break;
                    }
                }
                if (err == 0) {
                    err = pDevice->mRead(stream.mHandle, &tableCrc, 4, &done);
                    if (done != 4) {
                        err = 0x24;
                    } else {
                        stream.mOffset += 4;
                        if (tableCrc != stream.mCrcB) {
                            err = 0x2B;
                        }
                    }
                }
            }
        }
        if (pDir != 0) {
            TDbExtMemFree(lbl_803EBE8C, pDir, count * 8 + (count + 7) / 8);
        }
        if (err == 0) {
            err = pDevice->mClose(stream.mHandle);
        } else {
            pDevice->mClose(stream.mHandle);
        }
    }
    _TDbMemNotifyEnd();
    return err;
}

int fn_801FC5FC(TDbDatabase_801FAA00 *pDb, void *pName, TDbIODevice_t *pDevice)
{
    TDbIOFileHdr_801FC5FC hdr;
    TDbIOStream_801FA8C8 stream;
    TDbTable_t *pTable;
    unsigned int *pOffsets;
    unsigned int count;
    unsigned int n;
    unsigned int done;
    unsigned int base;
    int err;

    _TDbMemNotify(1, (int)pDb->mUnknown8, -1);
    memset(&hdr, 0, sizeof(hdr));
    count = 0;
    for (pTable = pDb->mTables; pTable != 0; pTable = pTable->mNext) {
        if (pTable->mFlags & 2) {
            count++;
        }
    }
    pOffsets = 0;
    err = TDbExtMemAlloc(lbl_803EBE8C, count * 8, &pOffsets);
    if (err != 0) {
        pOffsets = 0;
    }
    stream.mSize = fn_801FAA00(pDb, pOffsets) + (0x1C + count * 8);
    base = pDevice->mReserve(stream.mSize);
    hdr.mBase = base;
    stream.mEnd = hdr.mEnd = stream.mSize + base;
    stream.mUnknownC = 0;
    stream.mCrcB = -1;
    stream.mDevice = pDevice;
    stream.mCrcA = -1;
    stream.mOffset = 0;
    if (err == 0) {
        err = pDevice->mOpen(stream.mHandle, stream.mSize, pName, 0);
    }
    if (err == 0) {
        hdr.mMagic[0] = 'D';
        hdr.mMagic[1] = 'B';
        hdr.mUnknown2 = 0;
        hdr.mVersion = 8;
        hdr.mUnknown4 = 1;
        n = count;
        err = fn_801FA8C8(&stream, &hdr, sizeof(hdr));
        if (err == 0) {
            err = fn_801FA8C8(&stream, &n, 4);
            if (err == 0) {
                err = pDevice->mWrite(stream.mHandle, &stream.mCrcA, 4, &done);
                if (err == 0) {
                    if (done == 4) {
                        stream.mOffset += 4;
                    } else {
                        err = 0x24;
                    }
                }
            }
        }
        stream.mCrcA = -1;
        if (err == 0) {
            err = fn_801FA8C8(&stream, pOffsets, count * 8);
            TDbExtMemFree(lbl_803EBE8C, pOffsets, count * 8);
            pOffsets = 0;
            if (err == 0) {
                err = pDevice->mWrite(stream.mHandle, &stream.mCrcA, 4, &done);
                if (err == 0) {
                    if (done == 4) {
                        stream.mOffset += 4;
                    } else {
                        err = 0x24;
                    }
                }
                if (err == 0) {
                    for (pTable = pDb->mTables; pTable != 0; pTable = pTable->mNext) {
                        if ((pTable->mFlags & 2) && (err = fn_801FBD2C(pTable, &stream)) != 0) {
                            break;
                        }
                    }
                }
            }
        }
        if (pOffsets) {
            TDbExtMemFree(lbl_803EBE8C, pOffsets, count * 8);
            pOffsets = 0;
        }
        if (err == 0) {
            err = pDevice->mClose(stream.mHandle);
        } else {
            pDevice->mClose(stream.mHandle);
        }
    } else if (pOffsets) {
        TDbExtMemFree(lbl_803EBE8C, pOffsets, count * 8);
    }
    _TDbMemNotifyEnd();
    return err;
}

unsigned int fn_801FC950(void *pData, unsigned int size, unsigned int crc)
{
    unsigned char *p;

    if (lbl_802F9548[1] == 0) {
        fn_801FA870();
        crc = -1;
    }
    for (p = pData; size != 0; size--) {
        crc = (crc << 8) ^ lbl_802F9548[(crc >> 24) ^ *p++];
    }
    return crc;
}
