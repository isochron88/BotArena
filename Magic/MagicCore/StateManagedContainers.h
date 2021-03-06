#pragma once

/*

	Counted pointer containers:

		    CCountedPtrContainer
				    │  │
					│  д
					│  │
		            │  [SpecializedContainer] (e.g. CCountedCardContainer)
		  (default) д  │
					│  │  CStateSupport
					│  д  │
					│  │  д
					│  │  │
	 CCountedPtrContainer_ (overrides modification methods in Container;
									     implements Push, Pop and Peek)


	Value containers:

			  [ValueContainer]
				    │  │
					│  д
					│  │
		            │  [SpecializedContainer] (e.g. CPreventionContainer)
		  (default) д  │
					│  │  CStateSupport
					│  д  │
					│  │  д
					│  │  │
		CValueContainerBase_ (overrides modification methods in Container)
				    │           │
				    д           д
				    │           │
CValueContainer_     [SpecializedStateSupportContainer] (for implementing custom push, pop and peek logic)
(implements Push, Pop and Peek)
*/

//____________________________________________________________________________
//
#define ENUM_CONTAINER_STATE_SUPPORT(C) public: C(PVOID value) : m_Value((Enum)(DWORD)value) {} \
	operator PVOID() const { return (PVOID)m_Value; }

//____________________________________________________________________________
//
template <class V, class C = CCountedPtrContainer<V> >
class CCountedPtrContainer_
	: public C
	, public CStateSupport
{
public:
	virtual ~CCountedPtrContainer_()
	{
		RemoveSavedState();
	}

protected:
	OVERRIDE(void, AddImpl)(V* pContained)		
	{ 
		NotifyStateChanging();

		__super::AddImpl(pContained);

		NotifyStateChanged();
	}

	OVERRIDE2(void, AddImpl)(const CCountedPtrContainer& c)
	{
		NotifyStateChanging();

		__super::AddImpl(c);

		NotifyStateChanged();
	}

	OVERRIDE(void, InsertAtImpl)(int nIndex, V* pContained)
	{
		NotifyStateChanging();

		__super::InsertAtImpl(nIndex, pContained);

		NotifyStateChanged();
	}

	OVERRIDE2(void, InsertAtImpl)(int nIndex, const CCountedPtrContainer& c)
	{
		NotifyStateChanging();

		__super::InsertAtImpl(nIndex, c);

		NotifyStateChanged();
	}

	OVERRIDE(void, SetAtImpl)(int nIndex, V* pContained)
	{
		NotifyStateChanging();

		__super::SetAtImpl(nIndex, pContained);

		NotifyStateChanged();
	}
	
	OVERRIDE(void, RemoveAtImpl)(int nIndex)
	{ 
		NotifyStateChanging();

		__super::RemoveAtImpl(nIndex);

		NotifyStateChanged();
	}

	OVERRIDE(bool, RemoveImpl)(V* pContained)
	{
		int nIndex = __super::FindIndex(pContained);
		if (nIndex == -1)
			return false;

		NotifyStateChanging();

		__super::RemoveAtImpl(nIndex);

		NotifyStateChanged();

		return true;
	}

	OVERRIDE(void, RemoveAllImpl)()
	{ 
		if (!__super::size())
			return;

		NotifyStateChanging();

		__super::RemoveAllImpl(); 

		NotifyStateChanged();
	}

private:
	PRIVATE_OVERRIDE(void, Push)(StateStack& stateStack)
	{
		int nCount = GetSize();
		for (int i = nCount - 1; i >= 0; --i)
		{
			V* pContained = CCountedPtrContainer::GetAt(i).GetPointer();
			stateStack.AddTail(pContained);
			pContained->AddRef();
		}

		stateStack.AddTail((PVOID)nCount);
	}

	PRIVATE_OVERRIDE(void, Pop)()
	{
		__super::RemoveAllImpl();

		int nCount = (int)m_Stack.RemoveTail();
		for (int i = 0; i < nCount; ++i)
		{
			V* pContained = (V*)m_Stack.RemoveTail();
			__super::AddImpl(pContained);
			pContained->Release();
		}
	}

	PRIVATE_OVERRIDE(void, Peek)()
	{
		__super::RemoveAllImpl();

		StateStack::Position pos = m_Stack.GetTailPosition();

		int nCount = (int)m_Stack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			V* pContained = (V*)m_Stack.GetPrev(pos);
			__super::AddImpl(pContained);
		}
	}

#ifdef THINK_HISTORY
	PRIVATE_OVERRIDE(void, Cleanup)(const StateStack& stateStack, StateStack::Position pos)
	{
		int nCount = (int)stateStack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			V* pContained = (V*)stateStack.GetPrev(pos);
			if (pContained)
				pContained->Release();
		}
	}
#endif
};

//____________________________________________________________________________
//
template <class V, class C = CValueContainer<V> >
class CValueContainerBase_
	: public C
	, public CStateSupport
{
protected:
	OVERRIDE(void, AddImpl)(const V& c)		
	{ 
		NotifyStateChanging();

		__super::AddImpl(c);

		NotifyStateChanged();
	}

	OVERRIDE2(void, AddImpl)(const CValueContainer& c)
	{
		NotifyStateChanging();

		__super::AddImpl(c);

		NotifyStateChanged();
	}

	OVERRIDE(void, InsertAtImpl)(int nIndex, const V& c)
	{
		NotifyStateChanging();

		__super::InsertAtImpl(nIndex, c);

		NotifyStateChanged();
	}

	OVERRIDE2(void, InsertAtImpl)(int nIndex, const CValueContainer& c)
	{
		NotifyStateChanging();

		__super::InsertAtImpl(nIndex, c);

		NotifyStateChanged();
	}

	OVERRIDE(void, SetAtImpl)(int nIndex, const V& c)
	{
		NotifyStateChanging();

		__super::SetAtImpl(nIndex, c);

		NotifyStateChanged();
	}
	
	OVERRIDE(void, RemoveAtImpl)(int nIndex)
	{ 
		NotifyStateChanging();

		__super::RemoveAtImpl(nIndex);

		NotifyStateChanged();
	}

	OVERRIDE(bool, RemoveImpl)(const V& c)
	{
		int nIndex = FindIndex(c);
		if (nIndex == -1)
			return false;

		NotifyStateChanging();

		__super::RemoveAtImpl(nIndex);

		NotifyStateChanged();

		return true;
	}

	OVERRIDE(void, RemoveAllImpl)()
	{ 
		if (!__super::size())
			return;

		NotifyStateChanging();

		__super::RemoveAllImpl(); 

		NotifyStateChanged();
	}
};

//____________________________________________________________________________
//
template <class V, class C = CValueContainer<V> >
class CValueContainer_
	: public CValueContainerBase_<V, C>
{
public:
	virtual ~CValueContainer_()
	{
		RemoveSavedState();
	}

private:
	PRIVATE_OVERRIDE(void, Push)(StateStack& stateStack)
	{
		int nCount = GetSize();
		for (int i = nCount - 1; i >= 0; --i)
		{
			stateStack.AddTail((PVOID)GetAt(i));
		}

		stateStack.AddTail((PVOID)nCount);
	}

	PRIVATE_OVERRIDE(void, Pop)()
	{
		C::RemoveAllImpl();

		int nCount = (int)m_Stack.RemoveTail();
		for (int i = 0; i < nCount; ++i)
		{
			V c = (V)m_Stack.RemoveTail();
			C::AddImpl(c);
		}
	}

	PRIVATE_OVERRIDE(void, Peek)()
	{
		C::RemoveAllImpl();

		StateStack::Position pos = m_Stack.GetTailPosition();

		int nCount = (int)m_Stack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			V c = (V)m_Stack.GetPrev(pos);
			C::AddImpl(c);
		}
	}
};

typedef CValueContainer_<DWORD> CDWordArray_;

//____________________________________________________________________________
//
template <class V, class C = CPtrContainer<V> >
class CPtrContainerBase_
	: public C
	, public CStateSupport
{
protected:
	OVERRIDE(void, AddImpl)(V* c)		
	{ 
		NotifyStateChanging();

		__super::AddImpl(c);

		NotifyStateChanged();
	}

	OVERRIDE2(void, AddImpl)(const CPtrContainer& c)
	{
		NotifyStateChanging();

		__super::AddImpl(c);

		NotifyStateChanged();
	}

	OVERRIDE(void, InsertAtImpl)(int nIndex, V* c)
	{
		NotifyStateChanging();

		__super::InsertAtImpl(nIndex, c);

		NotifyStateChanged();
	}

	OVERRIDE2(void, InsertAtImpl)(int nIndex, const CPtrContainer& c)
	{
		NotifyStateChanging();

		__super::InsertAtImpl(nIndex, c);

		NotifyStateChanged();
	}

	OVERRIDE(void, SetAtImpl)(int nIndex, V* c)
	{
		NotifyStateChanging();

		__super::SetAtImpl(nIndex, c);

		NotifyStateChanged();
	}
	
	OVERRIDE(void, RemoveAtImpl)(int nIndex)
	{ 
		NotifyStateChanging();

		__super::RemoveAtImpl(nIndex);

		NotifyStateChanged();
	}

	OVERRIDE(bool, RemoveImpl)(V* c)
	{
		int nIndex = FindIndex(c);
		if (nIndex == -1)
			return false;

		NotifyStateChanging();

		__super::RemoveAtImpl(nIndex);

		NotifyStateChanged();

		return true;
	}

	OVERRIDE(void, RemoveAllImpl)()
	{ 
		if (!__super::size())
			return;

		NotifyStateChanging();

		__super::RemoveAllImpl(); 

		NotifyStateChanged();
	}
};

//____________________________________________________________________________
//
template <class V, class C = CPtrContainer<V> >
class CPtrContainer_
	: public CPtrContainerBase_<V, C>
{
public:
	virtual ~CPtrContainer_()
	{
		RemoveSavedState();
	}

private:
	PRIVATE_OVERRIDE(void, Push)(StateStack& stateStack)
	{
		int nCount = GetSize();
		for (int i = nCount - 1; i >= 0; --i)
		{
			stateStack.AddTail((PVOID)GetAt(i));
		}

		stateStack.AddTail((PVOID)nCount);
	}

	PRIVATE_OVERRIDE(void, Pop)()
	{
		C::RemoveAllImpl();

		int nCount = (int)m_Stack.RemoveTail();
		for (int i = 0; i < nCount; ++i)
		{
			V* c = (V*)m_Stack.RemoveTail();
			C::AddImpl(c);
		}
	}

	PRIVATE_OVERRIDE(void, Peek)()
	{
		C::RemoveAllImpl();

		StateStack::Position pos = m_Stack.GetTailPosition();

		int nCount = (int)m_Stack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			V* c = (V*)m_Stack.GetPrev(pos);
			C::AddImpl(c);
		}
	}
};

//____________________________________________________________________________
//
template <class K, class V, class C = CDictionary<K, V> >
class CDictionaryBase_
	: public C
	, public CStateSupport
{
protected:
	OVERRIDE(void, SetImpl)(const K& k, const V& v)			
	{
		NotifyStateChanging();

		__super::SetImpl(k, v);

		NotifyStateChanged();
	}

	OVERRIDE2(void, SetImpl)(const CDictionary& dictionary)
	{
		NotifyStateChanging();

		__super::SetImpl(dictionary);

		NotifyStateChanged();
	}

	OVERRIDE(bool, RemoveImpl)(const K& k)						
	{  
		NotifyStateChanging();

		bool bResult = __super::RemoveImpl(k);

		NotifyStateChanged();

		return bResult;
	}

	OVERRIDE(void, RemoveAllImpl)()								
	{
		if (!__super::size())
			return;

		NotifyStateChanging();

		__super::RemoveAllImpl();

		NotifyStateChanged();
	}
};

//____________________________________________________________________________
//
template <class K, class V, class C = CDictionary<K, V> >
class CDictionary_
	: public CDictionaryBase_<K, V, C>
{
public:
	virtual ~CDictionary_()
	{
		RemoveSavedState();
	}

private:
	PRIVATE_OVERRIDE(void, Push)(StateStack& stateStack)
	{
		for (Iterator it = Begin(); it != End(); ++it)
		{
			stateStack.AddTail((PVOID)it->first);	// K
			stateStack.AddTail((PVOID)it->second);	// V
		}

		stateStack.AddTail((PVOID)GetSize());
	}

	PRIVATE_OVERRIDE(void, Pop)()
	{
		C::RemoveAllImpl();

		int nCount = (int)m_Stack.RemoveTail();
		for (int i = 0; i < nCount; ++i)
		{
			V v = (V)m_Stack.RemoveTail();
			K k = (K)m_Stack.RemoveTail();
			C::SetImpl(k, v);
		}
	}

	PRIVATE_OVERRIDE(void, Peek)()
	{
		C::RemoveAllImpl();

		StateStack::Position pos = m_Stack.GetTailPosition();

		int nCount = (int)m_Stack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			V v = (V)m_Stack.GetPrev(pos);
			K k = (K)m_Stack.GetPrev(pos);
			C::SetImpl(k, v);
		}
	}
};

//____________________________________________________________________________
//
template <class K, class C = CSet<K> >
class CSetBase_
	: public C
	, public CStateSupport
{
protected:
	OVERRIDE(void, SetImpl)(const K& k)			
	{
		if (Contains(k))
			return;

		NotifyStateChanging();

		__super::SetImpl(k);

		NotifyStateChanged();
	}

	OVERRIDE2(void, SetImpl)(const CSet& aset)
	{
		if (Contains(aset))
			return;

		NotifyStateChanging();

		__super::SetImpl(aset);

		NotifyStateChanged();
	}

	OVERRIDE(bool, RemoveImpl)(const K& k)						
	{  
		if (!Contains(k))
			return false;

		NotifyStateChanging();

		bool bResult = __super::RemoveImpl(k);

		NotifyStateChanged();

		return bResult;
	}

	OVERRIDE(void, RemoveAllImpl)()								
	{
		if (!__super::size())
			return;

		NotifyStateChanging();

		__super::RemoveAllImpl();

		NotifyStateChanged();
	}
};

//____________________________________________________________________________
//
template <class K, class C = CSet<K> >
class CSet_
	: public CSetBase_<K, C>
{
public:
	virtual ~CSet_()
	{
		RemoveSavedState();
	}

private:
	PRIVATE_OVERRIDE(void, Push)(StateStack& stateStack)
	{
		for (Iterator it = Begin(); it != End(); ++it)
		{
			stateStack.AddTail((PVOID)*it);	// K
		}

		stateStack.AddTail((PVOID)GetSize());
	}

	PRIVATE_OVERRIDE(void, Pop)()
	{
		C::RemoveAllImpl();

		int nCount = (int)m_Stack.RemoveTail();
		for (int i = 0; i < nCount; ++i)
		{
			K k = (K)m_Stack.RemoveTail();
			C::SetImpl(k);
		}
	}

	PRIVATE_OVERRIDE(void, Peek)()
	{
		C::RemoveAllImpl();

		StateStack::Position pos = m_Stack.GetTailPosition();

		int nCount = (int)m_Stack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			K k = (K)m_Stack.GetPrev(pos);
			C::SetImpl(k);
		}
	}
};

//____________________________________________________________________________
//
template <class K, class C = CCountedPtrSet<K> >
class CCountedPtrSet_
	: public C
	, public CStateSupport
{
public:
	virtual ~CCountedPtrSet_()
	{
		RemoveSavedState();
	}

protected:
	OVERRIDE(void, SetImpl)(K* k)			
	{
		if (Contains(k))
			return;

		NotifyStateChanging();

		__super::SetImpl(k);

		NotifyStateChanged();
	}

	OVERRIDE2(void, SetImpl)(const CCountedPtrSet& aset)
	{
		if (Contains(aset))
			return;

		NotifyStateChanging();

		__super::SetImpl(aset);

		NotifyStateChanged();
	}

	OVERRIDE(bool, RemoveImpl)(K* k)						
	{  
		if (!Contains(k))
			return false;

		NotifyStateChanging();

		bool bResult = __super::RemoveImpl(k);

		NotifyStateChanged();

		return bResult;
	}

	OVERRIDE(void, RemoveAllImpl)()								
	{
		if (!__super::size())
			return;

		NotifyStateChanging();

		__super::RemoveAllImpl();

		NotifyStateChanged();
	}

private:
	PRIVATE_OVERRIDE(void, Push)(StateStack& stateStack)
	{
		for (Iterator it = Begin(); it != End(); ++it)
		{
			stateStack.AddTail((PVOID)it->GetPointer());	// K
			it->AddRef();
		}

		stateStack.AddTail((PVOID)GetSize());
	}

	PRIVATE_OVERRIDE(void, Pop)()
	{
		C::RemoveAllImpl();

		int nCount = (int)m_Stack.RemoveTail();
		for (int i = 0; i < nCount; ++i)
		{
			K* k = (K*)m_Stack.RemoveTail();
			C::SetImpl(k);
			k->Release();
		}
	}

	PRIVATE_OVERRIDE(void, Peek)()
	{
		C::RemoveAllImpl();

		StateStack::Position pos = m_Stack.GetTailPosition();

		int nCount = (int)m_Stack.GetPrev(pos);
		for (int i = 0; i < nCount; ++i)
		{
			K* k = (K*)m_Stack.GetPrev(pos);
			C::SetImpl(k);
		}
	}
};
