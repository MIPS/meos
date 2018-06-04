<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
<xsl:output method="text" omit-xml-declaration="yes"/>
<xsl:param name="modules" select="''"/>
<xsl:param name="fns" select="''"/>
<xsl:variable name="smallcase" select="'abcdefghijklmnopqrstuvwxyz'" />
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />

<xsl:template match="manual">
<xsl:text>
/*
* This is an automatically generated file. Do Not Edit.
*
* Outer debug wrapper for MEOS
*
*/

#include "meos/debug/dbg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEOS_NO_FUNCS

</xsl:text><xsl:apply-templates select="module" mode="definitions"/>
<xsl:apply-templates select="module" mode="wrappers"/>
<xsl:text>
char *_DBG_symnames[] = {
</xsl:text>
<xsl:apply-templates select="module" mode="symnames"/>
<xsl:text>
NULL, NULL
};

#ifdef __cplusplus
}
#endif
</xsl:text>
</xsl:template>

<xsl:template match="module" mode="definitions">
<xsl:if test="translate(@prefix, $uppercase, $smallcase)!='dbg' and $modules='' or contains($modules,translate(@prefix, $uppercase, $smallcase))">
<xsl:text>#include "meos/</xsl:text><xsl:value-of select="translate(@name, $uppercase, $smallcase)"/><xsl:text>/</xsl:text><xsl:value-of select="translate(@prefix, $uppercase, $smallcase)"/><xsl:text>.h"&#010;</xsl:text>
<xsl:apply-templates select="interface" mode="definitions"/>
</xsl:if>
</xsl:template>


<xsl:template match="interface" mode="definitions">
	<xsl:apply-templates select="func" mode="definitions"/>
</xsl:template>


<xsl:template match="module" mode="wrappers">
<xsl:if test="translate(@prefix, $uppercase, $smallcase)!='dbg' and $modules='' or contains($modules,translate(@prefix, $uppercase, $smallcase))">
<xsl:apply-templates select="interface" mode="wrappers"/>
</xsl:if>
</xsl:template>

<xsl:template match="interface" mode="wrappers">
        <xsl:apply-templates select="func" mode="wrappers"/>
</xsl:template>

<xsl:template match="func" mode="definitions">
	<xsl:if test="$fns='' or contains($fns, @name)">
	<xsl:if test="position()=1">
/* Function Prototypes */
#ifdef __STDC__
	</xsl:if>
	<xsl:if test="@vis!='virtual' and @vis!='virpriv'">
	<xsl:apply-templates select="return"/>
	<xsl:text disable-output-escaping="yes">&#032;_WRAPPER_</xsl:text>
	<xsl:apply-templates select="@name"/>
	<xsl:text>(</xsl:text>
	<xsl:apply-templates select="arg | varargs| voidargs" mode="definitions"/>)
	<xsl:if test="@return='N'">
		<xsl:text>__attribute__ ((noreturn))</xsl:text>
	</xsl:if>;
    </xsl:if>
	<xsl:if test="position()=last()">
		<xsl:text disable-output-escaping="yes">&#010;#endif /* __STDC__ */&#010;</xsl:text>
    </xsl:if>
</xsl:if>
</xsl:template>

	<xsl:template match="return" mode="definitions">
		<xsl:apply-templates select="@type" mode="definitions"/>
	</xsl:template>
	<xsl:template match="arg" mode="definitions">
		<xsl:apply-templates select="@type" mode="definitions"/>
		<xsl:if test="substring(@type,string-length(@type))!='*'">		<xsl:text disable-output-escaping="yes">&#032;</xsl:text></xsl:if>

		<xsl:apply-templates select="@name" mode="definitions"/>
		<xsl:if test="not(position()=last())">
			<xsl:text disable-output-escaping="yes">,&#032;</xsl:text>
		</xsl:if>
	</xsl:template>

	<xsl:template match="varargs" mode="definitions">
		<xsl:text>...</xsl:text>
	</xsl:template>
	<xsl:template match="voidargs"  mode="definitions">
		<xsl:text>void</xsl:text>
	</xsl:template>
	<xsl:template match="tab">
	<xsl:text disable-output-escaping="yes">&#032;&#032;&#032;&#032;</xsl:text>
	</xsl:template>
	<xsl:template match="nl"><xsl:text disable-output-escaping="yes">&#010;</xsl:text>
	</xsl:template>

<xsl:template match="limitations" mode="precondition">
	<xsl:apply-templates select="precondition"/>
</xsl:template>
<xsl:template match="limitations" mode="postcondition">
	<xsl:apply-templates select="postcondition"/>
</xsl:template>

<xsl:template match="precondition">
<xsl:text>	DBG_assert(</xsl:text><xsl:value-of select="."/><xsl:text>, "Precondition </xsl:text><xsl:value-of select="."/><xsl:text> unsatisfied");</xsl:text>
</xsl:template>
<xsl:template match="postcondition">
<xsl:text>	DBG_assert(</xsl:text><xsl:value-of select="."/><xsl:text>, "Postcondition </xsl:text><xsl:value-of select="."/><xsl:text> unsatisfied");</xsl:text>
</xsl:template>

<xsl:template match="arg" mode="pass">
	<xsl:if test="contains(@name, '[')"><xsl:value-of select="substring-before(@name, '[')"/></xsl:if>
	<xsl:if test="not(contains(@name, '['))"><xsl:value-of select="@name"/></xsl:if>
	<xsl:if test="not(position()=last())">
		<xsl:text disable-output-escaping="yes">,&#032;</xsl:text>
	</xsl:if>
</xsl:template>
<xsl:template match="voidargs" mode="pass"></xsl:template>

<xsl:template match="arg" mode="decl">
	<xsl:apply-templates select="@type"/>
	<xsl:if test="substring(@type,string-length(@type))!='*'"><xsl:text disable-output-escaping="yes">&#032;</xsl:text></xsl:if>
	<xsl:apply-templates select="@name"/>
	<xsl:if test="not(position()=last())">
		<xsl:text disable-output-escaping="yes">,&#032;</xsl:text>
	</xsl:if>
</xsl:template>
<xsl:template match="voidargs" mode="decl">
	<xsl:text>void</xsl:text>
</xsl:template>

<xsl:template match="arg" mode="par">
		<xsl:text disable-output-escaping="yes">,&#032;(uintptr_t)</xsl:text><xsl:apply-templates select="@name"/>
</xsl:template>

<xsl:template match="arg" mode="obtr">
	<xsl:if test="@traceable='Y'">
		<xsl:if test="contains(@name, '[')"><xsl:value-of select="substring-before(@name, '[')"/></xsl:if>
		<xsl:if test="not(contains(@name, '['))"><xsl:value-of select="@name"/></xsl:if>
		<xsl:text disable-output-escaping="yes">,&#032;</xsl:text>
	</xsl:if>
</xsl:template>
<xsl:template match="voidargs" mode="obtr"></xsl:template>

<xsl:template match="arg" mode="hw">
	<xsl:if test="contains($mode,'h')">DBG_RTTValue(<xsl:if test="contains(@name, '[')"><xsl:value-of select="substring-before(@name, '[')"/></xsl:if><xsl:if test="not(contains(@name, '['))"><xsl:value-of select="@name"/></xsl:if>);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
</xsl:template>


<xsl:template match="return">
	<xsl:apply-templates select="@type"/>
</xsl:template>

<xsl:template match="return" mode="obtr">
	<xsl:if test="@traceable='Y'">
			<xsl:text disable-output-escaping="yes">_RESULT ,&#032;</xsl:text>
	</xsl:if>
</xsl:template>

<xsl:template match="func" mode="wrappers">
<xsl:if test="$fns='' or contains($fns, @name)">
<xsl:if test="count(varargs)=0">
<xsl:if test="@vis!='virtual' and @vis!='virpriv'">
<xsl:if test="normalize-space(return/@type)='void'">
<xsl:text>void </xsl:text><xsl:apply-templates select="@name"/>(<xsl:apply-templates select="arg | voidargs" mode="decl"/>)
{
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">if (DBG_traceObjects(<xsl:apply-templates select="arg" mode="obtr"/> NULL)) {<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'h')">DBG_RTT((((uintptr_t)<xsl:apply-templates select="@name"/>) >> 2));<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text><xsl:apply-templates select="arg" mode="hw"/></xsl:if>
	<xsl:if test="contains($mode,'t')">DBG_wrapEnter(<xsl:apply-templates select="@name"/>, <xsl:value-of select="count(arg)"/><xsl:apply-templates select="arg" mode="par"/>, NULL);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">}<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'c')"><xsl:if test="count(limitations/precondition)!=0"><xsl:apply-templates select="limitations" mode="precondition"/><xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'v')">PARACHECK();<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	_WRAPPER_<xsl:apply-templates select="@name"/>(<xsl:apply-templates select="arg | voidargs" mode="pass"/>);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text>
	<xsl:if test="contains($mode,'v')">PARACHECK();<xsl:text disable-output-escaping="yes"></xsl:text></xsl:if>
	<xsl:if test="contains($mode,'c')"><xsl:if test="count(limitations/postcondition)!=0"><xsl:apply-templates select="limitations" mode="postcondition"/><xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">if (DBG_traceObjects(<xsl:apply-templates select="arg" mode="obtr"/> NULL)) {<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'h')">DBG_RTT((((uintptr_t)<xsl:apply-templates select="@name"/>) >> 2) | 0x40000000);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="contains($mode,'t')">DBG_wrapExit(<xsl:apply-templates select="@name"/>, 0, NULL);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">}</xsl:if></xsl:if>
}

</xsl:if>
<xsl:if test="normalize-space(return/@type)!='void'">
<xsl:apply-templates select="return"/><xsl:text> </xsl:text><xsl:apply-templates select="@name"/>(<xsl:apply-templates select="arg | voidargs" mode="decl"/>)
{
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">if (DBG_traceObjects(<xsl:apply-templates select="arg" mode="obtr"/> NULL)) {<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'h')">DBG_RTT((((uintptr_t)<xsl:apply-templates select="@name"/>) >> 2));<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="contains($mode,'t')">DBG_wrapEnter(<xsl:apply-templates select="@name"/>, <xsl:value-of select="count(arg)"/><xsl:apply-templates select="arg" mode="par"/>, NULL);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">}<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'c')"><xsl:if test="count(limitations/precondition)!=0"><xsl:apply-templates select="limitations" mode="precondition"/><xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'v')">PARACHECK();<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:apply-templates select="return"/> _RESULT = _WRAPPER_<xsl:apply-templates select="@name"/>(<xsl:apply-templates select="arg | voidargs" mode="pass"/>);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text>
	<xsl:if test="contains($mode,'v')">PARACHECK();<xsl:text disable-output-escaping="yes"></xsl:text></xsl:if>
<xsl:if test="contains($mode,'c')"><xsl:if test="count(limitations/postcondition)!=0"><xsl:apply-templates select="limitations" mode="postcondition"/><xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">if (DBG_traceObjects(<xsl:apply-templates select="arg" mode="obtr"/> <xsl:apply-templates select="return" mode="obtr"/> NULL)) {<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if></xsl:if>
	<xsl:if test="contains($mode,'h')">DBG_RTTPair((((uintptr_t)<xsl:apply-templates select="@name"/>) >> 2) | 0x40000000, (uint32_t)_RESULT);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="contains($mode,'t')">DBG_wrapExit(<xsl:apply-templates select="@name"/>, 1, (uintptr_t)_RESULT);<xsl:text disable-output-escaping="yes">&#010;&#x9;</xsl:text></xsl:if>
	<xsl:if test="arg[@traceable='Y']"><xsl:if test="contains($mode,'o')">}</xsl:if></xsl:if>

	return _RESULT;
}
</xsl:if>
</xsl:if>
</xsl:if>
</xsl:if>
</xsl:template>

<xsl:template match="module" mode="symnames">
	<xsl:apply-templates select="interface" mode="symnames"/>
</xsl:template>
<xsl:template match="interface" mode="symnames">
	<xsl:apply-templates select="func" mode="symnames"/>
</xsl:template>
<xsl:template match="func" mode="symnames"><xsl:if test="@vis!='virtual' and @vis!='virpriv'">
	(char*)<xsl:apply-templates select="@name"/>,"<xsl:apply-templates select="@name"/>",
</xsl:if></xsl:template>

</xsl:stylesheet>
