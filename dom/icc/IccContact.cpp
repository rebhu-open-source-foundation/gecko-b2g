/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "IccContact.h"
// #include "mozilla/dom/ContactsBinding.h"
#include "nsReadableUtils.h"

using namespace mozilla::dom::icc;

// using mozilla::dom::mozContact;

NS_IMPL_ISUPPORTS(IccContact, nsIIccContact)

// /* static */ nsresult
// IccContact::Create(mozContact& aMozContact, nsIIccContact** aIccContact)
// {
//   *aIccContact = nullptr;
//   ErrorResult er;

//   // Id
//   nsAutoString id;
//   aMozContact.GetId(id, er);
//   nsresult rv = er.StealNSResult();
//   NS_ENSURE_SUCCESS(rv, rv);
//   // Names
//   Nullable<nsTArray<nsString>> names;
//   names = aMozContact.names;
//   rv = er.StealNSResult();
//   NS_ENSURE_SUCCESS(rv, rv);
//   if (names.IsNull()) {
//     // Set as Empty nsTarray<nsString> for IccContact constructor.
//     names.SetValue();
//   }

//   // Numbers
//   Nullable<nsTArray<ContactTelField>> nullableNumberList;
//   aMozContact.GetTel(nullableNumberList, er);
//   rv = er.StealNSResult();
//   NS_ENSURE_SUCCESS(rv, rv);
//   nsTArray<nsString> numbers;
//   if (!nullableNumberList.IsNull()) {
//     const nsTArray<ContactTelField>& numberList = nullableNumberList.Value();
//     for (uint32_t i = 0; i < numberList.Length(); i++) {
//       if (numberList[i].mValue.WasPassed()) {
//         numbers.AppendElement(numberList[i].mValue.Value());
//       }
//     }
//   }

//   // Emails
//   Nullable<nsTArray<ContactField>> nullableEmailList;
//   aMozContact.GetEmail(nullableEmailList, er);
//   rv = er.StealNSResult();
//   NS_ENSURE_SUCCESS(rv, rv);
//   nsTArray<nsString> emails;
//   if (!nullableEmailList.IsNull()) {
//     const nsTArray<ContactField>& emailList = nullableEmailList.Value();
//     for (uint32_t i = 0; i < emailList.Length(); i++) {
//       if (emailList[i].mValue.WasPassed()) {
//         emails.AppendElement(emailList[i].mValue.Value());
//       }
//     }
//   }

//   nsCOMPtr<nsIIccContact> iccContact = new IccContact(id,
//                                                       names.Value(),
//                                                       numbers,
//                                                       emails);
//   iccContact.forget(aIccContact);
//   return NS_OK;
// }

/*static*/ nsresult
IccContact::Create(const nsAString& aId,
                   const nsTArray<nsString>& aNames,
                   const nsTArray<nsString>& aNumbers,
                   const nsTArray<nsString>& aEmails,
                   nsIIccContact** aIccContact)
{
  *aIccContact = nullptr;

  nsCOMPtr<nsIIccContact> iccContact = new IccContact(aId,
                                                      aNames,
                                                      aNumbers,
                                                      aEmails);
  iccContact.forget(aIccContact);

  return NS_OK;
}

IccContact::IccContact(const nsAString& aId,
                       const nsTArray<nsString>& aNames,
                       const nsTArray<nsString>& aNumbers,
                       const nsTArray<nsString>& aEmails)
  : mId(aId),
    mNames(aNames.Clone()),
    mNumbers(aNumbers.Clone()),
    mEmails(aEmails.Clone())
{
}

// nsIIccInfo implementation.

NS_IMETHODIMP IccContact::GetId(nsAString & aId)
{
  aId = mId;
  return NS_OK;
}

#define ICCCONTACT_IMPL_GET_CONTACT_FIELD(_field)                        \
  NS_IMETHODIMP IccContact::Get##_field(nsTArray<nsString>& a##_field) { \
    a##_field = m##_field.Clone();                                               \
    return NS_OK;                                                        \
  }

ICCCONTACT_IMPL_GET_CONTACT_FIELD(Names)
ICCCONTACT_IMPL_GET_CONTACT_FIELD(Numbers)
ICCCONTACT_IMPL_GET_CONTACT_FIELD(Emails)

#undef ICCCONTACT_GET_CONTACT_FIELD
