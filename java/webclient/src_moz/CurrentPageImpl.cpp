/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Mark Lin <mark.lin@eng.sun.com>
 *               Mark Goddard
 *               Ed Burns <edburns@acm.org>
 *               Ashutosh Kulkarni <ashuk@eng.sun.com>
 *               Kyle Yuan <kyle.yuan@sun.com>
 */

/*
 * CurrentPageImpl.cpp
 */

#include "org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl.h"

#include "ns_util.h"
#include "rdf_util.h"
#include "NativeBrowserControl.h"
#include "EmbedWindow.h"

#include "nsIWebBrowser.h"
#include "nsIWebBrowserFind.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsIDOMDocumentRange.h"
#include "nsIHistoryEntry.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIURI.h"

#include "nsCRT.h"

JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeCopyCurrentSelectionToSystemClipboard
(JNIEnv *env, jobject obj, jint nativeBCPtr)
{
    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    
    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null nativeBCPtr passed nativeGetSelection");
        return;
    }
    
    nsresult rv = nativeBrowserControl->mWindow->CopySelection();
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get Selection from browser");
        return;
    }

}

JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeGetSelection
(JNIEnv *env, jobject obj, jint nativeBCPtr, jobject selection)
{
    NativeBrowserControl *nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;

    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null nativeBCPtr passed nativeGetSelection");
        return;
    }

    if (selection == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null Selection object passed to nativeGetSelection");
        return;
    }
    nsresult rv = nativeBrowserControl->mWindow->GetSelection(env,
                                                              selection);
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get Selection from browser");
        return;
    }

    
}

JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeHighlightSelection
(JNIEnv *env, jobject obj, jint nativeBCPtr, jobject startContainer, jobject endContainer, jint startOffset, jint endOffset)
{
    NativeBrowserControl *nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    
    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null nativeBCPtr passed to nativeHighlightSelection");
        return;
    }
    
    if (env != nsnull && startContainer != nsnull && endContainer != nsnull &&
        startOffset > -1 && endOffset > -1) {
        nsresult rv = nsnull;
        
        // resolve ptrs to the nodes
        jclass nodeClass = env->FindClass("org/mozilla/dom/NodeImpl");
        if (!nodeClass) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }
        jfieldID nodePtrFID = env->GetFieldID(nodeClass, "p_nsIDOMNode", "J");
        if (!nodePtrFID) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }
        
        // get the nsIDOMNode representation of the start and end containers
        nsIDOMNode* node1 = (nsIDOMNode*)
            env->GetLongField(startContainer, nodePtrFID);

        nsIDOMNode* node2 = (nsIDOMNode*)
            env->GetLongField(endContainer, nodePtrFID);
        
        if (!node1 || !node2) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }
        
        // Get the DOM window and document
        nsCOMPtr<nsIDOMWindow> domWindow;
        nsCOMPtr<nsIWebBrowser> webBrowser;
        nsCOMPtr<nsIDOMDocument> domDocument;
        nativeBrowserControl->mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

        if (NS_FAILED(rv) || !webBrowser) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }
    
        // get the content DOM window for that web browser
        rv = webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));

        if (NS_FAILED(rv) || !domWindow )  {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }

        rv = domWindow->GetDocument(getter_AddRefs(domDocument));
        if (NS_FAILED(rv) || !domDocument )  {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }

        // Get the selection object of the DOM window
        nsCOMPtr<nsISelection> selection;
        rv = domWindow->GetSelection(getter_AddRefs(selection));
        if (NS_FAILED(rv) || selection == nsnull) {
            ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
            return;
        }
        
        nsCOMPtr<nsIDOMDocumentRange> docRange(do_QueryInterface(domDocument));
        if (docRange) {
            nsCOMPtr<nsIDOMRange> range;
            rv = docRange->CreateRange(getter_AddRefs(range));

            if (range) {
                rv = range->SetStart(node1, startOffset);
                if (NS_FAILED(rv))  {
                    ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
                    return;
                }

                rv = range->SetEnd(node2, endOffset);
                if (NS_FAILED(rv))  {
                    ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
                    return;
                }

                rv = selection->AddRange(range);
                if (NS_FAILED(rv))  {
                    ::util_ThrowExceptionToJava(env, "Exception: nativeHighlightSelection");
                    return;
                }
            }
        }
    }
}    

JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeClearAllSelections
(JNIEnv *env, jobject obj, jint nativeBCPtr)
{
    NativeBrowserControl *nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;

    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null nativeBCPtr passed to nativeClearAllSelections");
        return;
    }
    nsresult rv;

    // Get the DOM window and document
    nsCOMPtr<nsIDOMWindow> domWindow;
    nsCOMPtr<nsIWebBrowser> webBrowser;
    rv = nativeBrowserControl->mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

    if (NS_FAILED(rv) || !webBrowser) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeClearAllSelections");
        return;
    }

    // get the content DOM window for that web browser
    rv = webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    
    if (NS_FAILED(rv) || !domWindow )  {
        ::util_ThrowExceptionToJava(env, "Exception: nativeClearAllSelections");
        return;
    }

    nsCOMPtr<nsISelection> selection;
    rv = domWindow->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(rv) || !selection) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeClearAllSelections");
        return;
    }
    
    rv = selection->RemoveAllRanges();
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: nativeClearAllSelections");
        return;
    }
    
}

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeFind
 * Signature: (Ljava/lang/String;ZZ)V
 */
JNIEXPORT jboolean JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeFind
(JNIEnv *env, jobject obj, jint nativeBCPtr, jstring searchString, jboolean forward, jboolean matchCase)
{
    
    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    
    
    jstring searchStringGlobalRef = (jstring) ::util_NewGlobalRef(env, searchString);
    if (!searchStringGlobalRef) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't create global ref for search string");
        return JNI_FALSE;
    }
    nsresult rv = NS_ERROR_NULL_POINTER;
    
    // get the web browser
    nsCOMPtr<nsIWebBrowser> webBrowser;
    nativeBrowserControl->mWindow->GetWebBrowser(getter_AddRefs(webBrowser));
    nsCOMPtr<nsIWebBrowserFind> findComponent;
    nsCOMPtr<nsIInterfaceRequestor>
        findRequestor(do_GetInterface(webBrowser));
    
    rv = findRequestor->GetInterface(NS_GET_IID(nsIWebBrowserFind),
                                     getter_AddRefs(findComponent));
    
    if (NS_FAILED(rv) || nsnull == findComponent)  {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get find component");
        return JNI_FALSE;
    }
  
    PRUnichar * srchString = nsnull;
    
    srchString = (PRUnichar *) ::util_GetStringChars(env,
                                                     searchStringGlobalRef);
    // Check if String is NULL
    if (nsnull == srchString) {
        ::util_DeleteGlobalRef(env, searchStringGlobalRef);
        
        return JNI_FALSE;
    }

    rv = findComponent->SetSearchString(srchString);
    if (NS_FAILED(rv))  {
        ::util_ThrowExceptionToJava(env, "Exception: Can't set search string.");
        return JNI_FALSE;
    }
    findComponent->SetFindBackwards((PRBool) forward == JNI_FALSE);
    findComponent->SetMatchCase((PRBool) matchCase == JNI_TRUE);
    PRBool found = PR_TRUE;
    rv = findComponent->FindNext(&found);

    ::util_ReleaseStringChars(env, searchStringGlobalRef, srchString);
    ::util_DeleteGlobalRef(env, searchStringGlobalRef);

    return found ? JNI_TRUE : JNI_FALSE;
    
}

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeFindNext
 * Signature: (Z)V
 */
JNIEXPORT jboolean JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeFindNext
(JNIEnv *env, jobject obj, jint nativeBCPtr)
{

  NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    nsresult rv = NS_ERROR_NULL_POINTER;
    
    // get the web browser
    nsCOMPtr<nsIWebBrowser> webBrowser;
    nativeBrowserControl->mWindow->GetWebBrowser(getter_AddRefs(webBrowser));
    nsCOMPtr<nsIWebBrowserFind> findComponent;
    nsCOMPtr<nsIInterfaceRequestor>
        findRequestor(do_GetInterface(webBrowser));
    
    rv = findRequestor->GetInterface(NS_GET_IID(nsIWebBrowserFind),
                                     getter_AddRefs(findComponent));
    
    if (NS_FAILED(rv) || nsnull == findComponent)  {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get find component");
        return JNI_FALSE;
    }
  
    PRBool found = PR_TRUE;
    rv = findComponent->FindNext(&found);

    return found ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeGetCurrentURL
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeGetCurrentURL
(JNIEnv *env, jobject obj, jint nativeBCPtr)
{
    JNIEnv  *   pEnv = env;
    jobject     jobj = obj;
    char    *   charResult = nsnull;
    PRInt32 currentIndex;
    jstring     urlString = nsnull;
    nsresult rv = NS_ERROR_NULL_POINTER;

    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;

    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null nativeBCPtr passed to raptorWebShellGetURL");
        return nsnull;
    }

    nsCOMPtr<nsISHistory> sHistory;
    rv = nativeBrowserControl->mNavigation->GetSessionHistory(getter_AddRefs(sHistory));
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get SessionHistory");
        return nsnull;
    }
    
    rv = sHistory->GetIndex(&currentIndex);
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get current index from SessionHistory");
        return nsnull;
    }
    
    nsIHistoryEntry * Entry;
    nsIURI *URI;
    rv = sHistory->GetEntryAtIndex(currentIndex, PR_FALSE, &Entry);
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get history entry for current index");
        return nsnull;
    }
    
    rv = Entry->GetURI(&URI);
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get URI from History Entry");
        return nsnull;
    }
    
    nsCString urlSpecString;
    
    rv = URI->GetSpec(urlSpecString);
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get url spec from URI");
        return nsnull;
    }
    charResult = ToNewCString(urlSpecString);
    
    if (charResult != nsnull) {
        urlString = ::util_NewStringUTF(env, (const char *) charResult);
    }
    else {
        ::util_ThrowExceptionToJava(env, "raptorWebShellGetURL Exception: GetURL() returned NULL");
        return nsnull;
    }
    
    nsMemory::Free(charResult);
    
    return urlString;
}

JNIEXPORT jobject JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeGetDOM
(JNIEnv *env, jobject obj, jint nativeBCPtr)
{
    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    jobject result = nsnull;
    jlong documentLong = nsnull;
    jclass clazz = nsnull;
    jmethodID mid = nsnull;

    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null nativeBCPtr passed to raptorWebShellGetDOM");
        return nsnull;
    }

    // get the web browser
    nsCOMPtr<nsIWebBrowser> webBrowser;
    nativeBrowserControl->mWindow->GetWebBrowser(getter_AddRefs(webBrowser));
    
    // get the content DOM window for that web browser
    nsCOMPtr<nsIDOMWindow> domWindow;
    nsCOMPtr<nsIDOMDocument> domDocument;
    webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    if (!domWindow) {
        return nsnull;
    }
    domWindow->GetDocument(getter_AddRefs(domDocument));
    if (!domDocument) {
        return nsnull;
    }
    documentLong = (jlong) domDocument.get();

    if (nsnull == (clazz = ::util_FindClass(env,
                                            "org/mozilla/dom/DOMAccessor"))) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get DOMAccessor class");
        return nsnull;
    }
    if (nsnull == (mid = env->GetStaticMethodID(clazz, "getNodeByHandle",
                                                "(J)Lorg/w3c/dom/Node;"))) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't get DOM Node.");
        return nsnull;
    }

    result = (jobject) util_CallStaticObjectMethodlongArg(env, clazz, mid, 
                                                          documentLong);

    return result;
}

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeGetSource
 * Signature: ()Ljava/lang/String;
 */

/* PENDING(ashuk): remove this from here and in the motif directory
JNIEXPORT jstring JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeGetSource
(JNIEnv * env, jobject jobj)
{
    jstring result = nsnull;

    return result;
}
*/


/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeGetSourceBytes
 * Signature: ()[B
 */

/* PENDING(ashuk): remove this from here and in the motif directory
JNIEXPORT jbyteArray JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeGetSourceBytes
(JNIEnv * env, jobject jobj, jint nativeBCPtr, jboolean viewMode)
{


  NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;

  if (nativeBrowserControl->initComplete) {
      wsViewSourceEvent * actionEvent =
          new wsViewSourceEvent(nativeBrowserControl->docShell, ((JNI_TRUE == viewMode)? PR_TRUE : PR_FALSE));
      PLEvent       * event       = (PLEvent*) *actionEvent;

      ::util_PostEvent(nativeBrowserControl, event);
  }

    jbyteArray result = nsnull;
    return result;
}
*/


/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeResetFind
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeResetFind
(JNIEnv * env, jobject obj, jint nativeBCPtr)
{
  NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;

}

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativeSelectAll
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativeSelectAll
(JNIEnv * env, jobject obj, jint nativeBCPtr)
{
    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    
    if (nativeBrowserControl == nsnull) {
        ::util_ThrowExceptionToJava(env, "Exception: null passed to nativeSelectAll");
        return;
    }
    nsresult rv = nativeBrowserControl->mWindow->SelectAll();
    if (NS_FAILED(rv)) {
        ::util_ThrowExceptionToJava(env, "Exception: Can't selectAll");
        return;
    }
}

#if 0 // convenience

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativePrint
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativePrint
(JNIEnv * env, jobject obj, jint nativeBCPtr)
{
    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    if (nativeBrowserControl->initComplete) {
        wsPrintEvent * actionEvent = new wsPrintEvent(nativeBrowserControl);
        PLEvent         * event       = (PLEvent*) *actionEvent;
        ::util_PostEvent(nativeBrowserControl, event);
    }
}

/*
 * Class:     org_mozilla_webclient_impl_wrapper_0005fnative_CurrentPageImpl
 * Method:    nativePrintPreview
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_org_mozilla_webclient_impl_wrapper_1native_CurrentPageImpl_nativePrintPreview
(JNIEnv * env, jobject obj, jint nativeBCPtr, jboolean preview)
{
    NativeBrowserControl* nativeBrowserControl = (NativeBrowserControl *) nativeBCPtr;
    if (nativeBrowserControl->initComplete) {
        wsPrintPreviewEvent * actionEvent = new wsPrintPreviewEvent(nativeBrowserControl, preview);
        PLEvent         * event       = (PLEvent*) *actionEvent;
        ::util_PostEvent(nativeBrowserControl, event);
    }
}

# endif // if 0
