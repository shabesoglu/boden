#ifndef BDN_Window_H_
#define BDN_Window_H_

#include <bdn/WindowBase.h>

#include <bdn/IView.h>

#include <bdn/WindowTextProperty.h>
#include <bdn/WindowVisibleProperty.h>

namespace bdn
{

class Window : public WindowBase, BDN_IMPLEMENTS IView
{
public:
	Window();	

	
	/** Searches for the child window with the specified handle.
		Returns null if no such child was found.*/
	P<Window> findChildByHwnd(HWND hwnd);


	Property<bool>& visible() override
	{
		return *_pVisible;
	}

	ReadProperty<bool>& visible() const override
	{
		return *_pVisible;
	}

protected:

	/** Creates the window.
	
		@param className the name of the window class. For custom classes, take a look at 
			WindowClassBase - it can provide assistance in registering new window classes.
		@param name the window name. Depending on the window class this can be a label, window title,
			or simply a hidden internal name.
		@param style window style flags. Google "MSDN Window Styles" to see a list
		@param exStyle extended window style flags. Google "MSDN Extended Window Styles" to see a list
		@param pParentWindow the window's parent, or null if it does not have one.
	*/
	void create(	Window* pParent,
					const String& className,
					const String& name,
					DWORD style,
					DWORD exStyle,					
					int x=0,
					int y=0,
					int width=0,
					int height=0 );

	
	/** Called after the window handle has been set up. This is intended for further initialization,
		for example of the window object's properties.
		*/
	virtual void finishInit();
	


	/** This is called by the parent window when it receives a message that actually applies
		to this window.
		For example, Windows sends WM_COMMAND messages for button clicks to the button's parent,
		rather than the button. The parent then forwards the message to the button
		by calling its handleParentMessage function.
		*/
	virtual void handleParentMessage(MessageContext& context, HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);


	/** Returns the child window that the specified message should be forwarded to.*/
	virtual P<Window> findChildForMessage(UINT message, WPARAM wParam, LPARAM lParam);


	void handleMessage(MessageContext& context, HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) override;
	
protected:

	void notifyDestroy() override;
	
	// weak pointer by design.
	Window*						_pParentWeak;
	
	std::map<HWND, P<Window> >	_childMap;

	P<WindowTextProperty>		_pText;
	P<WindowVisibleProperty>	_pVisible;	

		
	// hide the createBase function
	void createBase(	const String& className,
						const String& name,
						DWORD style,
						DWORD exStyle,
						HWND parentHandle,
						int x=0,
						int y=0,
						int width=0,
						int height=0 ) = delete;
};

}

#endif

