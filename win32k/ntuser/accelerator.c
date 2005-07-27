/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/*
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 * Copyright 1997 Morten Welinder
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitAcceleratorImpl(VOID)
{
   return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupAcceleratorImpl(VOID)
{
   return(STATUS_SUCCESS);
}

int
STDCALL
NtUserCopyAcceleratorTable(
   HACCEL Table,
   LPACCEL Entries,
   int EntriesCount)
{
   PACCELERATOR_TABLE AcceleratorTable;
   NTSTATUS Status;
   int Ret;
   DECLARE_RETURN(int);

   DPRINT("Enter NtUserCopyAcceleratorTable\n");
   UserEnterExclusive();
                                           
   AcceleratorTable = UserGetAccelObject(Table);
   
   if (!AcceleratorTable)
   {
      SetLastNtError(STATUS_ACCESS_DENIED);
      RETURN(0);
   }

   if(Entries)
   {
      Ret = min(EntriesCount, AcceleratorTable->Count);
      Status = MmCopyToCaller(Entries, AcceleratorTable->Table, Ret * sizeof(ACCEL));
      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN(0);
      }
   }
   else
   {
      Ret = AcceleratorTable->Count;
   }

   RETURN(Ret);

CLEANUP:
   DPRINT("Leave NtUserCopyAcceleratorTable, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


PACCELERATOR_TABLE UserCreateAcceleratorTableObject(HACCEL* h)
{
   PVOID mem;
   PWINSTATION_OBJECT WinSta;
   
   /*
   Get process winsta? Or thread->desk->winsta? 
   can a process threads be assigned to different winstas (via desktops?)   
   */
   WinSta = IntGetWinStaObj();
   
   mem = ExAllocatePool(PagedPool, sizeof(ACCELERATOR_TABLE));
   if (!mem) return NULL;
   RtlZeroMemory(mem, sizeof(ACCELERATOR_TABLE));
   *h = UserAllocHandle(&WinSta->HandleTable, mem, USER_ACCELERATOR_TABLE);
   if (!*h){
      ExFreePool(mem);
      return NULL;
   }
   return mem;
}



HACCEL
STDCALL
NtUserCreateAcceleratorTable(
   LPACCEL Entries,
   SIZE_T EntriesCount)
{
   PACCELERATOR_TABLE AcceleratorTable;
   NTSTATUS Status;
   HACCEL Handle;
   DECLARE_RETURN(HACCEL);

   DPRINT("Enter NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d)\n",
          Entries, EntriesCount);
   UserEnterExclusive();

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(STATUS_ACCESS_DENIED);
      DPRINT1("E1\n");
      RETURN(FALSE);
   }

   AcceleratorTable = UserCreateAcceleratorTableObject(&Handle);
   if (AcceleratorTable == NULL)
   {

      SetLastNtError(STATUS_NO_MEMORY);
      DPRINT1("E2\n");
      RETURN( (HACCEL) 0);
   }

   AcceleratorTable->Count = EntriesCount;
   if (AcceleratorTable->Count > 0)
   {
      //FIXME: alloc as part of object itself??
      AcceleratorTable->Table = ExAllocatePoolWithTag(PagedPool, EntriesCount * sizeof(ACCEL), TAG_ACCEL);
      if (AcceleratorTable->Table == NULL)
      {
//         ObmCloseHandle(WindowStation->HandleTable, Handle);
         SetLastNtError(Status);
         DPRINT1("E3\n");
         RETURN( (HACCEL) 0);
      }

      Status = MmCopyFromCaller(AcceleratorTable->Table, Entries, EntriesCount * sizeof(ACCEL));
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(AcceleratorTable->Table);
//         ObmCloseHandle(WindowStation->HandleTable, Handle);
         SetLastNtError(Status);
         DPRINT1("E4\n");
         RETURN((HACCEL) 0);
      }
   }


   /* FIXME: Save HandleTable in a list somewhere so we can clean it up again */
   RETURN( (HACCEL) Handle);

CLEANUP:
   DPRINT("Leave NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d) = %x end\n",
          Entries, EntriesCount, Handle);
   UserLeave();
   END_CLEANUP;
}



BOOLEAN
STDCALL
NtUserDestroyAcceleratorTable(HACCEL Table)
{
   PACCELERATOR_TABLE AcceleratorTable;
   DECLARE_RETURN(BOOLEAN);
   /* FIXME: If the handle table is from a call to LoadAcceleratorTable, decrement it's
      usage count (and return TRUE).
   FIXME: Destroy only tables created using CreateAcceleratorTable.
    */

   DPRINT("Enter NtUserDestroyAcceleratorTable(Table %x)\n", Table);
   UserEnterExclusive();

   //FIXME
   AcceleratorTable = UserGetAccelObject(Table);

   if (!AcceleratorTable)
   {
      SetLastWin32Error(ERROR_INVALID_ACCEL_HANDLE);
//      ObDereferenceObject(WindowStation);
      DPRINT1("E2\n");
      RETURN(FALSE);
   }

//   ObmCloseHandle(WindowStation->HandleTable, Table);

   if (AcceleratorTable->Table != NULL)
   {
      ExFreePool(AcceleratorTable->Table);
   }

//   ObDereferenceObject(WindowStation);

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserDestroyAcceleratorTable(Table %x) = %i\n", Table,_ret_);
   UserLeave();
   END_CLEANUP;
}

static BOOLEAN
IntTranslateAccelerator(HWND hWnd,
                        UINT message,
                        WPARAM wParam,
                        LPARAM lParam,
                        BYTE fVirt,
                        WORD key,
                        WORD cmd)
{
   UINT mesg = 0;

   DPRINT("IntTranslateAccelerator(hWnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x)\n",
          hWnd, message, wParam, lParam, fVirt, key, cmd);

   if (wParam != key)
   {
      DPRINT("T0\n");
      return FALSE;
   }

   if (message == WM_CHAR)
   {
      if (!(fVirt & FALT) && !(fVirt & FVIRTKEY))
      {
         DPRINT("found accel for WM_CHAR: ('%c')\n", wParam & 0xff);
         goto found;
      }
   }
   else
   {
      if ((fVirt & FVIRTKEY) > 0)
      {
         INT mask = 0;
         DPRINT("found accel for virt_key %04x (scan %04x)\n",
                wParam, 0xff & HIWORD(lParam));

         DPRINT("NtUserGetKeyState(VK_SHIFT) = 0x%x\n",
                UserGetKeyState(VK_SHIFT));
         DPRINT("NtUserGetKeyState(VK_CONTROL) = 0x%x\n",
                UserGetKeyState(VK_CONTROL));
         DPRINT("NtUserGetKeyState(VK_MENU) = 0x%x\n",
                UserGetKeyState(VK_MENU));

         if (UserGetKeyState(VK_SHIFT) & 0x8000)
            mask |= FSHIFT;
         if (UserGetKeyState(VK_CONTROL) & 0x8000)
            mask |= FCONTROL;
         if (UserGetKeyState(VK_MENU) & 0x8000)
            mask |= FALT;
         if (mask == (fVirt & (FSHIFT | FCONTROL | FALT)))
            goto found;
         DPRINT("but incorrect SHIFT/CTRL/ALT-state\n");
      }
      else
      {
         if (!(lParam & 0x01000000))  /* no special_key */
         {
            if ((fVirt & FALT) && (lParam & 0x20000000))
            {                            /* ^^ ALT pressed */
               DPRINT("found accel for Alt-%c\n", wParam & 0xff);
               goto found;
            }
         }
      }
   }

   DPRINT("IntTranslateAccelerator(hWnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x) = FALSE\n",
          hWnd, message, wParam, lParam, fVirt, key, cmd);

   return FALSE;

found:
   if (message == WM_KEYUP || message == WM_SYSKEYUP)
      mesg = 1;
   else if (UserGetCaptureWindow())
      mesg = 2;
   else if (!UserIsWindowVisible(GetWnd(hWnd))) /* FIXME: WINE IsWindowEnabled == IntIsWindowVisible? */
      mesg = 3;
   else
   {
#if 0
      HMENU hMenu, hSubMenu, hSysMenu;
      UINT uSysStat = (UINT)-1, uStat = (UINT)-1, nPos;

      hMenu = (UserGetWindowLongW(hWnd, GWL_STYLE) & WS_CHILD) ? 0 : GetMenu(hWnd);//FIXME!
      hSysMenu = get_win_sys_menu(hWnd);

      /* find menu item and ask application to initialize it */
      /* 1. in the system menu */
      hSubMenu = hSysMenu;
      nPos = cmd;
      if(MENU_FindItem(&hSubMenu, &nPos, MF_BYCOMMAND))
      {
         IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hSysMenu, 0L);
         if(hSubMenu != hSysMenu)
         {
            nPos = MENU_FindSubMenu(&hSysMenu, hSubMenu);
            TRACE_(accel)("hSysMenu = %p, hSubMenu = %p, nPos = %d\n", hSysMenu, hSubMenu, nPos);
            IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, TRUE));
         }
         uSysStat = GetMenuState(GetSubMenu(hSysMenu, 0), cmd, MF_BYCOMMAND);
      }
      else /* 2. in the window's menu */
      {
         hSubMenu = hMenu;
         nPos = cmd;
         if(MENU_FindItem(&hSubMenu, &nPos, MF_BYCOMMAND))
         {
            IntSendMessage(hWnd, WM_INITMENU, (WPARAM)hMenu, 0L);
            if(hSubMenu != hMenu)
            {
               nPos = MENU_FindSubMenu(&hMenu, hSubMenu);
               TRACE_(accel)("hMenu = %p, hSubMenu = %p, nPos = %d\n", hMenu, hSubMenu, nPos);
               IntSendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, FALSE));
            }
            uStat = GetMenuState(hMenu, cmd, MF_BYCOMMAND);
         }
      }

      if (uSysStat != (UINT)-1)
      {
         if (uSysStat & (MF_DISABLED|MF_GRAYED))
            mesg=4;
         else
            mesg=WM_SYSCOMMAND;
      }
      else
      {
         if (uStat != (UINT)-1)
         {
            if (IsIconic(hWnd))
               mesg=5;
            else
            {
               if (uStat & (MF_DISABLED|MF_GRAYED))
                  mesg=6;
               else
                  mesg=WM_COMMAND;
            }
         }
         else
         {
            mesg=WM_COMMAND;
         }
      }
#else
      DPRINT1("menu search not implemented");
      mesg = WM_COMMAND;
#endif

   }

   if (mesg == WM_COMMAND)
   {
      DPRINT(", sending WM_COMMAND, wParam=%0x\n", 0x10000 | cmd);
      IntSendMessage(hWnd, mesg, 0x10000 | cmd, 0L);
   }
   else if (mesg == WM_SYSCOMMAND)
   {
      DPRINT(", sending WM_SYSCOMMAND, wParam=%0x\n", cmd);
      IntSendMessage(hWnd, mesg, cmd, 0x00010000L);
   }
   else
   {
      /*  some reasons for NOT sending the WM_{SYS}COMMAND message:
       *   #0: unknown (please report!)
       *   #1: for WM_KEYUP,WM_SYSKEYUP
       *   #2: mouse is captured
       *   #3: window is disabled
       *   #4: it's a disabled system menu option
       *   #5: it's a menu option, but window is iconic
       *   #6: it's a menu option, but disabled
       */
      DPRINT(", but won't send WM_{SYS}COMMAND, reason is #%d\n", mesg);
      if (mesg == 0)
      {
         DPRINT1(" unknown reason - please report!");
      }
   }

   DPRINT("IntTranslateAccelerator(hWnd %x, message %x, wParam %x, lParam %x, fVirt %d, key %x, cmd %x) = TRUE\n",
          hWnd, message, wParam, lParam, fVirt, key, cmd);

   return TRUE;
}

inline PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hCursor)
{
   PWINSTATION_OBJECT WinSta;
   WinSta = PsGetWin32Thread()->Desktop->WindowStation;
   return (PACCELERATOR_TABLE)UserGetObject(&WinSta->HandleTable, hCursor, USER_CURSOR_ICON );   
}


int
STDCALL
NtUserTranslateAccelerator(
   HWND hWnd,
   HACCEL Table,
   LPMSG Message)
{
   PWINSTATION_OBJECT WindowStation;
   PACCELERATOR_TABLE AcceleratorTable;
   ULONG i;
   DECLARE_RETURN(int);

   DPRINT("Enter NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p)\n",
          hWnd, Table, Message);
   UserEnterExclusive();

   if (hWnd == NULL)
      RETURN(0);

   if (Message == NULL)
   {
      SetLastNtError(STATUS_INVALID_PARAMETER);
      RETURN(0);
   }

   if (Table == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_ACCEL_HANDLE);
      RETURN(0);
   }

   if ((Message->message != WM_KEYDOWN) &&
         (Message->message != WM_SYSKEYDOWN) &&
         (Message->message != WM_SYSCHAR) &&
         (Message->message != WM_CHAR))
   {
      RETURN(0);
   }

   AcceleratorTable = UserGetAccelObject(Table);
   if (!AcceleratorTable)
   {
      SetLastWin32Error(ERROR_INVALID_ACCEL_HANDLE);
      RETURN(0);
   }

   /* FIXME: Associate AcceleratorTable with the current thread */

   for (i = 0; i < AcceleratorTable->Count; i++)
   {
      if (IntTranslateAccelerator(hWnd, Message->message, Message->wParam, Message->lParam,
                                  AcceleratorTable->Table[i].fVirt, AcceleratorTable->Table[i].key,
                                  AcceleratorTable->Table[i].cmd))
      {
         ObDereferenceObject(WindowStation);
         DPRINT("NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p) = %i end\n",
                hWnd, Table, Message, 1);
         RETURN(1);
      }
      if (((AcceleratorTable->Table[i].fVirt & 0x80) > 0))
      {
         break;
      }
   }

   RETURN(0);

CLEANUP:
   DPRINT("Leave NtUserTranslateAccelerator(hWnd %x, Table %x, Message %p) = %i end\n",
          hWnd, Table, Message, _ret_);
   UserLeave();
   END_CLEANUP;
}
