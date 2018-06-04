<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
<xsl:output method="text" omit-xml-declaration="yes"/>

<xsl:template match="module">
#ifndef <xsl:value-of select="@prefix"/>_H
#define <xsl:value-of select="@prefix"/>_H
/*
* This is an automatically generated file. Do Not Edit.
*
* Interface Definition for module: <xsl:value-of select="@name"/>
*
*/

#ifdef __cplusplus
extern "C" {
#endif

<xsl:apply-templates select="interface"/>

#ifdef __cplusplus
}
#endif

#endif /* #ifndef <xsl:value-of select="@prefix"/>_H */&#010;</xsl:template>
<xsl:template match="interface">
        <xsl:value-of select="prologue" disable-output-escaping="yes"/>
        <xsl:apply-templates select="include"/>
	<xsl:apply-templates select="define"/>
	<xsl:apply-templates select="typedef"/>
	<xsl:apply-templates select="var"/>
	<xsl:apply-templates select="func"/>
	<xsl:apply-templates select="postinclude"/>
	<xsl:value-of select="epilogue" disable-output-escaping="yes"/>
	<xsl:text disable-output-escaping="yes">&#010;</xsl:text>
</xsl:template>

<xsl:template match="include">
	<xsl:if test="position()=1">
/* included header files */
	</xsl:if>
	<xsl:if test="@lib='Y'">
#include <xsl:text disable-output-escaping="yes">&lt;</xsl:text><xsl:apply-templates/><xsl:text disable-output-escaping="yes">&gt;</xsl:text>
	</xsl:if>
	<xsl:if test="not(@lib='Y')">
#include "<xsl:apply-templates/>"
	</xsl:if>
</xsl:template>

<xsl:template match="define">
	<xsl:if test="position()=1">
/* Macro definitions */
	</xsl:if>
	<xsl:if test="@vis!='virtual'">
#define <xsl:text disable-output-escaping="yes">&#032;</xsl:text><xsl:apply-templates select="c"/>
	</xsl:if>
</xsl:template>

<xsl:template match="typedef">
	<xsl:if test="position()=1">
/* Type definitions */
	</xsl:if>
	<xsl:if test="not(@vis='virtual' or @vis='virpriv' or @vis='viranon')">
typedef<xsl:text disable-output-escaping="yes">&#032;</xsl:text>
	<xsl:apply-templates select="c"/><xsl:text disable-output-escaping="yes">&#032;</xsl:text><xsl:text>;</xsl:text>
	<xsl:if test="@vis='private'"><xsl:text disable-output-escaping="yes">&#032;</xsl:text>/* Private  */</xsl:if>
	<xsl:if test="@vis='anonymous'"><xsl:text disable-output-escaping="yes">&#032;</xsl:text>/* Anonymous */</xsl:if>
	</xsl:if>
</xsl:template>

<xsl:template match="var">
	<xsl:if test="position()=1">
/* External Variables */
#ifdef __STDC__
	</xsl:if>
extern<xsl:text disable-output-escaping="yes">&#032;</xsl:text>
	<xsl:value-of select="@type"/>
	<xsl:if test="substring(@type,string-length(@type))!='*'">
		<xsl:text disable-output-escaping="yes">&#032;</xsl:text>
	</xsl:if>
	<xsl:value-of select="@decl"/>
	<xsl:text>;</xsl:text>
	<xsl:if test="position()=last()">
		<xsl:text disable-output-escaping="yes">&#010;#endif /* __STDC__ */&#010;</xsl:text>
	</xsl:if>
</xsl:template>

<xsl:template match="func">
	<xsl:if test="position()=1">
/* Function Prototypes */
#ifndef MEOS_NO_FUNCS
#ifdef __STDC__
	</xsl:if>
	<xsl:if test="@vis!='virtual' and @vis!='virpriv'">
		<xsl:value-of select="@quals"/>
		<xsl:text disable-output-escaping="yes">&#032;</xsl:text>
		<xsl:apply-templates select="return"/>
		<xsl:text disable-output-escaping="yes">&#032;</xsl:text>
		<xsl:apply-templates select="@name"/>
		<xsl:text>(</xsl:text>
		<xsl:apply-templates select="arg | varargs| voidargs"/>)
		<xsl:if test="@return='N'">
			<xsl:text>__attribute__ ((noreturn))</xsl:text>
		</xsl:if>;
	</xsl:if>
	<xsl:if test="position()=last()">
		<xsl:text disable-output-escaping="yes">&#010;#endif /* __STDC__ */&#010;#endif /* MEOS_NO_FUNCS */&#010;</xsl:text>
	</xsl:if>
</xsl:template>

<xsl:template match="return">
	<xsl:apply-templates select="@type"/>
</xsl:template>

<xsl:template match="arg">
	<xsl:apply-templates select="@type"/>
	<xsl:if test="substring(@type,string-length(@type))!='*'">		<xsl:text disable-output-escaping="yes">&#032;</xsl:text></xsl:if>
	<xsl:apply-templates select="@name"/>
	<xsl:if test="not(position()=last())">
		<xsl:text disable-output-escaping="yes">,&#032;</xsl:text>
	</xsl:if>
</xsl:template>

<xsl:template match="varargs">
	<xsl:text>...</xsl:text>
</xsl:template>

<xsl:template match="voidargs">
	<xsl:text>void</xsl:text>
</xsl:template>

<xsl:template match="tab">
	<xsl:text disable-output-escaping="yes">&#032;&#032;&#032;&#032;</xsl:text>
</xsl:template>

<xsl:template match="nl"><xsl:text disable-output-escaping="yes">&#010;</xsl:text>
</xsl:template>

<xsl:template match="parahead">
	<xsl:text>PARAHEAD</xsl:text>
</xsl:template>
<xsl:template match="paratail">
	<xsl:text>PARATAIL</xsl:text>
</xsl:template>
<xsl:template match="postinclude">
	<xsl:if test="position()=1">
/* included header files */
	</xsl:if>
	<xsl:if test="@lib='Y'">
#include <xsl:text disable-output-escaping="yes">&lt;</xsl:text>
		<xsl:apply-templates/>
		<xsl:text disable-output-escaping="yes">&gt;</xsl:text>
	</xsl:if>
	<xsl:if test="not(@lib='Y')">
#include "<xsl:apply-templates/>"
	</xsl:if>
</xsl:template>

</xsl:stylesheet>
