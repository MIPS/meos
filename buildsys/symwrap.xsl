<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
<xsl:output method="text" omit-xml-declaration="yes"/>
<xsl:variable name="nl"><xsl:text>&#xA;</xsl:text></xsl:variable>
<xsl:variable name="smallcase" select="'abcdefghijklmnopqrstuvwxyz'" />
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
<xsl:param name="modules" select="''"/>
<xsl:param name="fns" select="''"/>

<xsl:template match="manual">
<xsl:apply-templates select="module"/>
</xsl:template>

<xsl:template match="module">
	<xsl:if test="translate(@prefix, $uppercase, $smallcase)!='dbg' and $modules='' or contains($modules,translate(@prefix, $uppercase, $smallcase))">
		<xsl:apply-templates select="interface"/>
	</xsl:if>
</xsl:template>
<xsl:template match="interface"><xsl:apply-templates select="func"/></xsl:template>

<xsl:template match="func"><xsl:if test="@name!='__cyg_profile_func_enter' and @name!='__cyg_profile_func_exit' and ($fns='' or contains($fns, @name))"><xsl:if test="count(varargs)=0"><xsl:if test="@vis!='virtual' and @vis!='virpriv' and @vis!='viranon'"><xsl:apply-templates select="@name"/><xsl:text> _WRAPPER_</xsl:text><xsl:apply-templates select="@name"/><xsl:value-of select="$nl"/></xsl:if></xsl:if></xsl:if></xsl:template>

</xsl:stylesheet>
