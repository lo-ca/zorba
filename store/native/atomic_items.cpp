	/* -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
 *
 *  $Id: item.h,v 1.1 2006/10/09 07:07:59 $
 *
 *	Copyright 2006-2007 FLWOR Foundation.
 *  Author: David Graf, Donald Kossmann, Tim Kraska
 *
 */

#include "atomic_items.h"
#include "../../runtime/zorba.h"
#include "../api/item.h"

namespace xqp
{
	/* start class AtomicItem */
	bool AtomicItem::isNode() const
	{
		return false;
	}
	bool AtomicItem::isAtomic() const
	{
		return true;
	}
	/* end class AtomicItem */

	/* start class QNameItem */
	QNameItem::QNameItem(xqp_string _namespace, xqp_string prefix, xqp_string localname)
		:
	strNamespace_(_namespace), strPrefix_(prefix), strLocal_(localname) {}
	
	xqp_string QNameItem::getQNameNamespace() const
	{
		return this->strNamespace_;
	}

	xqp_string QNameItem::getQNamePrefix() const
	{
		return this->strPrefix_;
	}

	xqp_string QNameItem::getQNameLocalName() const
	{
		return this->strLocal_;
	}
	
	qnamekey_t QNameItem::getQNameKey( ) const
	{
		return Item::createQNameKey(this->strNamespace_, this->strPrefix_, this->strLocal_);
	}

	sequence_type_t QNameItem::getType( ) const
	{
		return xs_qname;
	}

	Item_t QNameItem::getAtomizationValue( ) const
	{
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createQName(this->strNamespace_, this->strPrefix_, this->strLocal_);
	}
	
	bool QNameItem::equals(Item_t item) const {
		return (item->getQNameNamespace() == this->strNamespace_
						&& item->getQNameLocalName() == this->strLocal_);
	}
	
	Item_t QNameItem::getEBV( ) const
	{
		ZorbaErrorAlerts::error_alert (
				error_messages::FORG0006_INVALID_ARGUMENT_TYPE,
				error_messages::RUNTIME_ERROR,
				NULL,
				false,
				"Effective Boolean Value is not defined for QName!"
			);
		return NULL;
	}
	xqp_string QNameItem::getStringProperty( ) const
	{
		return this->strPrefix_ != "" ? this->strPrefix_ + ":" + this->strLocal_ : this->strLocal_;
	}
	xqp_string QNameItem::show() const
	{
		return "xs:qname(" + this->strNamespace_ + "," + this->strPrefix_ + "," + this->strLocal_ + ")";
	}
	/* end class QNameItem */

	/* start class StringItem */
	StringItem::StringItem ( xqp_string value ) :strValue_ ( value ) {}

	xqp_string StringItem::getStringValue() const
	{
		return this->strValue_;
	}

	sequence_type_t StringItem::getType() const
	{
		return xs_string;
	}

	Item_t StringItem::getAtomizationValue() const
	{
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createString(this->strValue_);
	}
	
	bool StringItem::equals(Item_t item) const {
		return item->getStringValue() == this->strValue_;
	}

	Item_t StringItem::getEBV() const
	{
		bool b = ! ( this->strValue_ == "" );
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createBoolean ( b );
	}

	xqp_string StringItem::getStringProperty() const
	{
		return this->strValue_;
	}
	
	xqp_string StringItem::show() const
	{
		return "xs:string(" + this->strValue_ + ")";
	}
	/* end class StringItem */

	/* start class DecimalItem */
	DecimalItem::DecimalItem ( long double value ) :value_ ( value ) {}

	long double DecimalItem::getDecimalValue() const
	{
		return this->value_;
	}

	sequence_type_t DecimalItem::getType() const
	{
		return xs_decimal;
	}

	Item_t DecimalItem::getAtomizationValue() const
	{
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createDecimal(this->value_);
	}
	
	bool DecimalItem::equals(Item_t item) const {
		return item->getDecimalValue() == this->value_;
	}

	Item_t DecimalItem::getEBV() const
	{
		bool b = ( this->value_ == 0 );
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createBoolean ( b );
	}

	xqp_string DecimalItem::getStringProperty() const
	{
		std::ostringstream tmp;
		tmp << this->value_;
		return xqp_string ( tmp.str() );
	}
	
	xqp_string DecimalItem::show() const
	{
		return "xs:decimal(" + this->getStringProperty() + ")";
	}
	/* end class DecimalItem */

	/* start class IntItem */
	IntItem::IntItem ( int value ) :value_ ( value ) {}
	
	int32_t IntItem::getIntValue() const
	{
		return this->value_;
	}
	
	long long IntItem::getIntegerValue() const
	{
		return static_cast<long long>(this->value_);
	}
	
	long double IntItem::getDecimalValue() const
	{
		return static_cast<long double>(this->value_);
	}
	
	sequence_type_t IntItem::getType() const
	{
		return xs_int;
	}
	
	Item_t IntItem::getAtomizationValue() const
	{
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createInt(this->value_);
	}
	
	bool IntItem::equals(Item_t item) const {
		return item->getIntValue() == this->value_;
	}
	
	Item_t IntItem::getEBV() const
	{
		bool b = ( this->value_ == 0 );
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createBoolean ( b );
	}
	
	xqp_string IntItem::getStringProperty() const
	{
		std::ostringstream tmp;
		tmp << this->value_;
		return xqp_string(tmp.str());
	}
	
	xqp_string IntItem::show() const {
		return "xs:int(" + this->getStringProperty() + ")";
	}
	/* end class IntItem */
	
	/* start class IntegerItem */
	IntegerItem::IntegerItem ( long long value ) :value_ ( value ) {}
	
	long long IntegerItem::getIntegerValue() const
	{
		return this->value_;
	}
	
	long double IntegerItem::getDecimalValue() const
	{
		return static_cast<long double>(this->value_);
	}
	
	sequence_type_t IntegerItem::getType() const
	{
		return xs_integer;
	}
	
	Item_t IntegerItem::getAtomizationValue() const
	{
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createInteger(this->value_);
	}
	
	bool IntegerItem::equals(Item_t item) const {
		return item->getIntegerValue() == this->value_;
	}
	
	Item_t IntegerItem::getEBV() const
	{
		bool b = (this->value_ == 0);
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createBoolean( b );
	}
	
	xqp_string IntegerItem::getStringProperty() const
	{
		std::ostringstream tmp;
		tmp << this->value_;
		return xqp_string(tmp.str());
	}
	
	xqp_string IntegerItem::show() const {
		return "xs:integer(" + this->getStringProperty() + ")";
	}
	/* end class IntegerItem */
	
	/* start class BooleanItem */
	BooleanItem::BooleanItem ( bool value ) :value_ ( value ) {}
	
	bool BooleanItem::getBooleanValue() const
	{
		return this->value_;
	}
	
	sequence_type_t BooleanItem::getType() const
	{
		return xs_boolean;
	}
	
	Item_t BooleanItem::getAtomizationValue() const
	{
		return zorba::getZorbaForCurrentThread()->getItemFactory()->createBoolean(this->value_);
	}
	
	bool BooleanItem::equals(Item_t item) const {
		return item->getBooleanValue() == this->value_;
	}
	
	Item_t BooleanItem::getEBV() const
	{
		return this->getAtomizationValue();
	}
	
	xqp_string BooleanItem::getStringProperty() const
	{
		if (this->value_)
			return "true";
		else
			return "false";
	}
	
	xqp_string BooleanItem::show() const
	{
		return "xs:boolean(" + this->getStringProperty() + ")";
	}
	/* end class BooleanItem */

}/* namespace xqp */
