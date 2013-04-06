#include <iostream>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <Synchapi.h>
using namespace std;

CRITICAL_SECTION CriticalSection;

class LRU
{
public:
	LRU(int size)
	{
		_head = _tail = NULL;
		_size = size;	
	}

	int GetElem(int i)
	{
		EnterCriticalSection(&CriticalSection); 
		int retValue;
		if(_hashMap.find(i) != _hashMap.end())
		{
			Element * found = _hashMap[i];
			retValue = found->Value;

			if(found == _tail)
			{				
				LeaveCriticalSection(&CriticalSection);
				return retValue;
			}

			//When there is only one item,should skip
			if(found == _head && found->Pre != NULL)			
				_head = found->Pre;

			//linked the found Element Pre and Next, if exist
			if(found->Pre != NULL)
				found->Pre->Next = found->Next;
			if(found->Next != NULL)
				found->Next->Pre = found->Pre;

			//Put the found Element as the tail,
			found->Next = _tail;
			found->Pre = NULL;
			if(_tail != NULL )
				_tail->Pre = found;
			_tail = found;
		}
		else
		{
			//if the size reach the max size,remove the head Element
			if(_hashMap.size() == _size)
			{
				Element * toDel = _head;
				int key =_head->Key;
				_head = _head->Pre;
				_head->Next = NULL;
				delete	toDel;
				_hashMap.erase(key);
			}

			///Put the New Element as the tail
			Element *newElement = new Element(i);
			newElement->Next = _tail;
			newElement->Pre = NULL;
			newElement->Value = GetFromDataSource(i);
			if(newElement->Next != NULL)
				newElement->Next->Pre = newElement; 	

			//Init the _head
			if(_hashMap.size() == 0 && _head == NULL)
				_head = newElement;

			_hashMap[i] = newElement;

			_tail = newElement;		

			retValue = newElement->Value;
		}

		LeaveCriticalSection(&CriticalSection);
		return retValue;
	}

private:
	struct Element
	{
		Element(int v):Key(v){}
		Element * Next ,* Pre;
		int Key;
		int Value;
	};

	int GetFromDataSource(int i)
	{
		return i;
	}
	unordered_map<int,Element*> _hashMap;

	Element *_head;
	Element * _tail;
	int _size;
};

#define MAX_THREADS 12
#define BUF_SIZE 255

DWORD WINAPI MyThreadFunction( LPVOID lpParam );
void ErrorHandler(LPTSTR lpszFunction);

int _tmain()
{
	if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 
		0x00000400) ) 
		return 1;


	DWORD   dwThreadIdArray[MAX_THREADS];
	HANDLE  hThreadArray[MAX_THREADS]; 

	LRU lru = LRU(10); 

	for( int i=0; i<MAX_THREADS; i++ )
	{
		hThreadArray[i] = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			MyThreadFunction,       // thread function name
			&lru,          // argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[i]);   // returns the thread identifier 


		if (hThreadArray[i] == NULL) 
		{
			ErrorHandler(TEXT("CreateThread"));
			ExitProcess(3);
		}
	} 

	// Wait until all threads have terminated.

	WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);

	// Close all thread handles and free memory allocations.

	for(int i=0; i<MAX_THREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
	}

	return 0;
}


DWORD WINAPI MyThreadFunction( LPVOID lpParam ) 
{ 
	LRU *lru= (LRU*) lpParam;

	while (true)
	{
		int random= rand();
		//cout << "input number is "<< random <<endl;
		int output = lru->GetElem(random);
		//cout<< "output number is "<< output <<endl;
	}

}

void ErrorHandler(LPTSTR lpszFunction) 
{ 
	// Retrieve the system error message for the last-error code.

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR) lpMsgBuf) + lstrlen((LPCTSTR) lpszFunction) + 40) * sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR) lpDisplayBuf, TEXT("Error"), MB_OK); 

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}