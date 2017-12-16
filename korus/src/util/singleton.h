#pragma once

//
// singleton.h
//
//  Created on: 2016-05-05
//      Author: zjg
//

template<class T>
class singleton
{
public:
	static T& instance()
	{ 
		if(!m_pInstance)
		{
			m_pInstance = new T;
		}
		return *m_pInstance;
	}
	static void uninstance()
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = nullptr;
		}
	}
protected:
	singleton(){};
private:
	static T* m_pInstance;
 };

template<class T>
T*	singleton<T>::m_pInstance = nullptr;

