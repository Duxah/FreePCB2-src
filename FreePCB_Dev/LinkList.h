#ifndef _LINKLIST_H /* [ */
#define _LINKLIST_H

#include <stddef.h>
#include <assert.h>

// Circular doubly linked list
//
// CDLinkList may be used as a mix-in class.
class CDLinkList
{
private:
	void _DLinkList_remove()
	{
		next->prev = prev;
		prev->next = next;
	}

public:
	CDLinkList * prev;
	CDLinkList * next;

public:
	CDLinkList()  { DLinkList_init(); }
	~CDLinkList() { _DLinkList_remove(); }

	void DLinkList_init()
	{
	  prev = next = this;
	}

	int isLinked() const
	{
		return (this != next);
	}

	void insert_after(CDLinkList *pDLL_Element)
	{
		ASSERT(!pDLL_Element->isLinked());

		next->prev = pDLL_Element;
		pDLL_Element->next = next;
		pDLL_Element->prev = this;
		next = pDLL_Element;
	}

	void insert_before(CDLinkList *pDLL_Element)
	{
		ASSERT(!pDLL_Element->isLinked());

		prev->next = pDLL_Element;
		pDLL_Element->prev = prev;
		pDLL_Element->next = this;
		prev = pDLL_Element;
	}

	void move_after(CDLinkList *pDLL_Element)
	{
		pDLL_Element->DLinkList_remove();
		insert_after(pDLL_Element);
	}

	void move_before(CDLinkList *pDLL_Element)
	{
		pDLL_Element->DLinkList_remove();
		insert_before(pDLL_Element);
	}

	void DLinkList_remove()
	{
		_DLinkList_remove();
		DLinkList_init();
	}
};

#endif /* !_LINKLIST_H ] */
