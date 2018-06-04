<?xml version="1.0" encoding="UTF-8"?>
<!--XSL stylesheet to convert module definitions into rst documentation-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xi="http://www.w3.org/2001/XInclude" xmlns:ext="http://exslt.org/common" extension-element-prefixes="ext">
	<xsl:output method="text"/>
	<xsl:variable name="h1src" select="'----------------------------------------------------------------------------------------------------'"/>
	<xsl:variable name="h2src" select="'===================================================================================================='"/>
	<xsl:variable name="h3src" select="'~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~'"/>
	<xsl:variable name="h4src" select="'++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++'"/>
	<xsl:variable name="nl"><xsl:text>&#xA;</xsl:text></xsl:variable>

	<xsl:template match="manual">
		<xsl:apply-templates select="child::*"/>
	</xsl:template>

	<xsl:template match="module">
		<xsl:text>.. _</xsl:text><xsl:value-of select="@name"/><xsl:text>-top:</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. class:: module</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>The </xsl:text><xsl:value-of select="@name"/><xsl:text> module</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="substring($h2src,1,string-length(@name)+11)"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>	:class: live</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>	"Module Description"</xsl:text><xsl:value-of select="$nl"/>
		<xsl:apply-templates select="imports" mode="toc"/>
		<xsl:text>	"Module Interface"</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="description"/>
		<xsl:apply-templates select="imports"/>
		<xsl:apply-templates select="interface"/>
	</xsl:template>

	<xsl:template match="imports">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Imported Modules</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>================</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../imports" mode="row"/><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>`Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="imports" mode="toc">
		<xsl:if test="position()=1">
			<xsl:text>	"Imported Modules"</xsl:text><xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="imports" mode="row">
		<xsl:text>	"</xsl:text><xsl:apply-templates select="current()" mode="normalize"/><xsl:text>"</xsl:text>
		<xsl:if test="position()!=last()"><xsl:value-of select="$nl"/></xsl:if>
	</xsl:template>

	<xsl:template match="idprefix">
		<xsl:apply-templates select="current()" mode="normalize"/>
	</xsl:template>

	<xsl:template match="cvsid">
		<xsl:apply-templates select="current()" mode="normalize"/>
	</xsl:template>

	<xsl:template match="reldate">
		<xsl:apply-templates select="current()" mode="normalize"/>
	</xsl:template>

	<xsl:template match="module/description">
		<xsl:text>.. class:: description</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Module Description</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>==================</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates mode="normalize"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="description">
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Description</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>+++++++++++</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates mode="normalize"/>
	</xsl:template>

	<xsl:template match="description" mode="normalize">
		<xsl:apply-templates mode="normalize"/>
	</xsl:template>

	<xsl:template match="par">
		<xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="par" mode="normalize">
		<xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="bold">
		<xsl:text>**</xsl:text>
		<xsl:apply-templates/>
		<xsl:text>**</xsl:text>
	</xsl:template>

	<xsl:template match="bold" mode="normalize">
		<xsl:text>**</xsl:text>
		<xsl:apply-templates mode="normalize"/>
		<xsl:text>**</xsl:text>
	</xsl:template>

	<xsl:template match="emph">
		<xsl:text>*</xsl:text>
		<xsl:apply-templates/>
		<xsl:text>*</xsl:text>
	</xsl:template>

	<xsl:template match="emph" mode="normalize">
		<xsl:text>*</xsl:text>
		<xsl:apply-templates mode="normalize"/>
		<xsl:text>*</xsl:text>
	</xsl:template>

	<xsl:template match="interface">
		<xsl:text>.. _</xsl:text><xsl:value-of select="ancestor::module[1]/@name"/><xsl:text>-interface:</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Module Interface</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>================</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>	:class: live</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:apply-templates select="include" mode="iftoc"/>
				<xsl:apply-templates select="define" mode="iftoc"/>
				<xsl:apply-templates select="typedef" mode="iftoc"/>
				<xsl:apply-templates select="var" mode="iftoc"/>
				<xsl:apply-templates select="func" mode="iftoc"/>
			</xsl:with-param>
		</xsl:call-template>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>`Top`__</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. __: `</xsl:text>
		<xsl:value-of select="ancestor::module[1]/@name"/>
		<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="include"/>
		<xsl:apply-templates select="define|typedef|var"/>
		<xsl:apply-templates select="func"/>
	</xsl:template>

	<xsl:template match="include">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Include Files</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>~~~~~~~~~~~~~</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../include" mode="row"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>`Interface`__, `Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-interface`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="include" mode="row">
		<xsl:text>	"``</xsl:text>
		<xsl:apply-templates mode="normalize"/>
		<xsl:text>``","</xsl:text>
		<xsl:if test="@lib='Y'">
			<xsl:text>Library</xsl:text>
		</xsl:if>
		<xsl:if test="@lib='N'">
			<xsl:text>Local</xsl:text>
		</xsl:if>
		<xsl:text>"</xsl:text>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="define|typedef[@vis!='private']|typedef[not(@vis)]|var">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Global Declarations</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>~~~~~~~~~~~~~~~~~~~</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../define" mode="global"/>
			<xsl:apply-templates select="../typedef" mode="global"/>
			<xsl:apply-templates select="../var" mode="global"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="typedef[@vis='private']"/>

	<xsl:template match="define" mode="global">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Macro Definitions</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>+++++++++++++++++</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>	:class: meos-defines</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../define" mode="row"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>`Interface`__, `Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-interface`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="define" mode="row">
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:text>"</xsl:text>
				<xsl:apply-templates select="c" mode="defcode"/>
				<xsl:text>","</xsl:text>
				<xsl:apply-templates select="current()" mode="normalize"/>
				<xsl:text>"</xsl:text>
				<xsl:value-of select="$nl"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="enum">
		<xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>	:class: meos-enums</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="child::*"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="enum" mode="normalize">
		<xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>	:class: meos-enums</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="child::*"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="item">
		<xsl:text>	"</xsl:text>
		<xsl:value-of select="@val"/>
		<xsl:text>","</xsl:text>
		<xsl:apply-templates select="current()" mode="normalize"/>
		<xsl:text>"</xsl:text>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="typedef[@vis!='private']|typedef[not(@vis)]" mode="global">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Type Definitions</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>++++++++++++++++</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>	:class: meos-typedefs</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../typedef" mode="row"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>`Interface`__, `Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-interface`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="typedef[@vis='private']" mode="global"/>

	<xsl:template match="typedef[@vis='public']|typedef[not(@vis)]" mode="row">
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:text>"</xsl:text>
				<xsl:apply-templates select="c" mode="typedefcode"/>
				<xsl:text>","Public.","</xsl:text>
				<xsl:apply-templates select="current()" mode="normalize"/>
				<xsl:text>"</xsl:text>
				<xsl:value-of select="$nl"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="typedef[@vis='anonymous']|typedef[@vis='viranon']" mode="row">
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:text>"</xsl:text>
				<xsl:apply-templates select="c" mode="typedefcode"/>
				<xsl:text>","Anonymous.","</xsl:text>
				<xsl:apply-templates select="current()" mode="normalize"/>
				<xsl:text>"</xsl:text>
				<xsl:value-of select="$nl"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="typedef[@vis='private']" mode="row">
		<!--
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:text>"</xsl:text>
				<xsl:apply-templates select="c" mode="typedefcode"/>
				<xsl:text>","Private.","</xsl:text>
				<xsl:apply-templates select="current()" mode="normalize"/>
				<xsl:text>"</xsl:text>
				<xsl:value-of select="$nl"/>
			</xsl:with-param>
		</xsl:call-template>
	-->
	</xsl:template>

	<xsl:template match="var" mode="global">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Variable Declarations</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>+++++++++++++++++++++</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>	:class: meos-variables</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../var" mode="row"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>`Interface`__, `Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-interface`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="var" mode="row">
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:text>"</xsl:text>
				<xsl:apply-templates select="current()" mode="varcode"/>
				<xsl:text>","</xsl:text>
				<xsl:apply-templates select="current()" mode="normalize"/>
				<xsl:text>"</xsl:text>
				<xsl:value-of select="$nl"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="func">
		<xsl:if test="position()=1">
			<xsl:text>.. _</xsl:text><xsl:value-of select="ancestor::module[1]/@name"/><xsl:text>-functions:</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:if test="count(../func[@vis!='virpriv' and @vis!='private'])>0">
				<xsl:text>Function Prototypes</xsl:text><xsl:value-of select="$nl"/>
				<xsl:text>~~~~~~~~~~~~~~~~~~~</xsl:text><xsl:value-of select="$nl"/>
				<xsl:value-of select="$nl"/>
				<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
				<xsl:value-of select="$nl"/>
				<xsl:apply-templates select="../func" mode="functoc"/>
				<xsl:value-of select="$nl"/>
				<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
				<xsl:value-of select="$nl"/>
			</xsl:if>
			<xsl:text>`Interface`__, `Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-interface`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../func" mode="funcdetail"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="func[@vis!='virpriv' and @vis!='private']|func[not(@vis)]" mode="functoc">
		<xsl:call-template name="indent">
			<xsl:with-param name="text">
				<xsl:text>"</xsl:text>
				<xsl:apply-templates select="." mode="funccode"/>
				<xsl:text>"</xsl:text>
				<xsl:value-of select="$nl"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>
	<xsl:template match="func[@vis='virpriv']|func[@vis='private']" mode="functoc"/>

	<xsl:template match="func[@vis!='virpriv' and @vis!='private']|func[not(@vis)]" mode="funcdetail">
		<xsl:if test="@vis!='private' and @vis!='virpriv'">
			<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:value-of select="@name"/><xsl:value-of select="$nl"/>
			<xsl:value-of select="substring($h3src,1,string-length(@name))"/><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:call-template name="code">
				<xsl:with-param name="content">
					<xsl:apply-templates select="return" mode="header"/>
					<xsl:value-of select="@name"/>
					<xsl:text>(</xsl:text>
					<xsl:apply-templates select="voidargs" mode="header"/>
					<xsl:apply-templates select="arg|varargs" mode="header"/>
					<xsl:text>)</xsl:text>
				</xsl:with-param>
				<xsl:with-param name="suffix" select="';'"/>
			</xsl:call-template><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="arg|varargs" mode="table"/>
			<xsl:apply-templates select="return"/>
			<xsl:apply-templates select="entry"/>
			<xsl:apply-templates select="exit"/>
			<xsl:apply-templates select="resources"/>
			<xsl:apply-templates select="description"/>
			<xsl:apply-templates select="limitations"/>
			<xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. class:: live</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>`Functions`__, `Interface`__, `Top`__</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-functions`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-interface`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>.. __: `</xsl:text>
			<xsl:value-of select="ancestor::module[1]/@name"/>
			<xsl:text>-top`_</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>
	<xsl:template match="func[@vis='virpriv']|func[@vis='private']" mode="funcdetail"/>

	<xsl:template match="arg" mode="header">
		<xsl:value-of select="@type"/>
		<xsl:if test="substring(@type,string-length(@type))!='*'">
			<xsl:text> </xsl:text>
		</xsl:if>
		<xsl:value-of select="@name"/>
		<xsl:if test="position()!=last()">
			<xsl:text>, </xsl:text>
		</xsl:if>
	</xsl:template>

	<xsl:template match="varargs" mode="header">
		<xsl:text>...</xsl:text>
	</xsl:template>

	<xsl:template match="voidargs" mode="header">
		<xsl:text>void</xsl:text>
	</xsl:template>

	<xsl:template name="quote">
		<xsl:param name="text" />
		<xsl:choose>
			<xsl:when test="contains($text, '&quot;')">
				<xsl:value-of select="substring-before($text,'&quot;')" />
				<xsl:value-of select="&quot;&quot;" />
				<xsl:call-template name="quote">
					<xsl:with-param name="text" select="substring-after($text,'&quot;')" />
				</xsl:call-template>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="$text" />
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="arg|varargs" mode="table">
		<xsl:if test="position()=1">
			<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>Arguments</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>+++++++++</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
			<xsl:text>	:class: meos-arguments</xsl:text><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
			<xsl:apply-templates select="../arg|../varargs" mode="row"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="arg" mode="row">
		<xsl:text>	"``</xsl:text>
		<xsl:if test="string-length(@type)>0">
			<xsl:value-of select="@type"/>
			<xsl:if test="substring(@type,string-length(@type))!='*'">
				<xsl:text> </xsl:text>
			</xsl:if>
		</xsl:if>
		<xsl:value-of select="@name"/>
		<xsl:text>``","</xsl:text>
		<xsl:call-template name="quote">
			<xsl:with-param name="text" select="normalize-space(.)"/>
		</xsl:call-template>
		<xsl:text>"</xsl:text><xsl:value-of select="$nl"/>
		<xsl:if test="position()=last()">
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="varargs" mode="row">
		<xsl:text>	"``...``","Optional arguments."</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="func/description/description">
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Description</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>+++++++++++</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="current()" mode="normalize"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="func/entry/entry">
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Entry Conditions</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>++++++++++++++++</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="current()" mode="normalize"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="func/exit/exit">
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Exit Conditions</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>+++++++++++++++</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="current()" mode="normalize"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="func/resources/resources">
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Resources</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>+++++++++</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="current()" mode="normalize"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="return">
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Returns ``</xsl:text>
		<xsl:value-of select="@type"/>
		<xsl:text>``</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="substring($h4src,1,string-length(@type)+19)"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:if test="string-length(.)">
			<xsl:apply-templates mode="normalize"/><xsl:value-of select="$nl"/>
			<xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="return" mode="header">
		<xsl:value-of select="@type"/>
		<xsl:if test="substring(@type,string-length(@type))!='*'">
			<xsl:text> </xsl:text>
		</xsl:if>
	</xsl:template>

	<xsl:template match="nl">
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="nl" mode="normalize">
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="tab">
		<xsl:text>	</xsl:text>
	</xsl:template>

	<xsl:template match="tab" mode="normalize">
		<xsl:text>	</xsl:text>
	</xsl:template>

	<xsl:template match="parahead">
		<xsl:text></xsl:text>
	</xsl:template>

	<xsl:template match="parahead" mode="normalize">
		<xsl:text></xsl:text>
	</xsl:template>

	<xsl:template match="paratail">
		<xsl:text></xsl:text>
	</xsl:template>

	<xsl:template match="paratail" mode="normalize">
		<xsl:text></xsl:text>
	</xsl:template>

	<xsl:template match="include" mode="iftoc">
		<xsl:if test="position()=1">
			<xsl:text>"Include Files"</xsl:text><xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="define" mode="iftoc">
		<xsl:if test="position()=1">
			<xsl:text>"Macro Definitions"</xsl:text><xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="typedef" mode="iftoc">
		<xsl:if test="position()=1">
			<xsl:text>"Type Definitions"</xsl:text><xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="var" mode="iftoc">
		<xsl:if test="position()=1">
			<xsl:text>"Public Variables"</xsl:text><xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>

	<xsl:template match="func[@vis!='virpriv' and @vis!='private']|func[not(@vis)]" mode="iftoc">
		<xsl:if test="position()=1">
			<xsl:text>"Function Prototypes"</xsl:text><xsl:value-of select="$nl"/>
		</xsl:if>
	</xsl:template>
	<xsl:template match="func[@vis='virpriv']|func[@vis='private']" mode="iftoc"/>

	<xsl:template match="c"/>

	<xsl:template match="c" mode="normalize"/>

	<xsl:template match="c" mode="showcode">
		<xsl:call-template name="code">
			<xsl:with-param name="content">
				<xsl:apply-templates/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="c" mode="typedefcode">
		<xsl:call-template name="code">
			<xsl:with-param name="prefix" select="'typedef '"/>
			<xsl:with-param name="content">
				<xsl:apply-templates mode="normalize"/>
			</xsl:with-param>
			<xsl:with-param name="suffix" select="';'"/>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="c" mode="defcode">
		<xsl:call-template name="code">
			<xsl:with-param name="prefix" select="'#define '"/>
			<xsl:with-param name="content">
				<xsl:apply-templates mode="normalize"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="var" mode="varcode">
		<xsl:call-template name="code">
			<xsl:with-param name="prefix" select="'extern '"/>
			<xsl:with-param name="content">
				<xsl:value-of select="@type"/>
				<xsl:if test="substring(@type,string-length(@type))!='*'">
					<xsl:text> </xsl:text>
				</xsl:if>
				<xsl:value-of select="@decl"/>
			</xsl:with-param>
			<xsl:with-param name="suffix" select="';'"/>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="func[@vis!='virpriv' and @vis!='private']|func[not(@vis)]" mode="funccode">
		<xsl:if test="@vis!='private' and @vis!='virpriv'">
			<xsl:call-template name="code">
				<xsl:with-param name="content">
					<xsl:apply-templates select="return" mode="header"/>
					<xsl:value-of select="@name"/>
					<xsl:text>(</xsl:text>
					<xsl:apply-templates select="voidargs" mode="header"/>
					<xsl:apply-templates select="arg|varargs" mode="header"/>
					<xsl:text>)</xsl:text>
				</xsl:with-param>
				<xsl:with-param name="suffix" select="';'"/>
			</xsl:call-template>
		</xsl:if>
	</xsl:template>
	<xsl:template match="func[@vis='virpriv']|func[@vis='private']" mode="funccode"/>

	<xsl:template match="limitations">
		<xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. class:: function-definition</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>Limitations</xsl:text><xsl:value-of select="$nl"/>
		<xsl:text>+++++++++++</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. class:: function-limitations</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:text>.. csv-table::</xsl:text><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
		<xsl:apply-templates select="current()" mode="normalize"/><xsl:value-of select="$nl"/>
		<xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template match="arglimit">
		<xsl:text>	"``</xsl:text>
		<xsl:value-of select="@argname"/>
		<xsl:text>``","</xsl:text>
		<xsl:if test="@num!='0'">
			<xsl:value-of select="@num"/> number format.<br/>
		</xsl:if>
		<xsl:if test="string-length(@min)!=0">Minimum: <tt>
				<xsl:value-of select="@min"/>
			</tt>.<br/>
		</xsl:if>
		<xsl:if test="string-length(@max)!=0">Maximum: <tt>
				<xsl:value-of select="@max"/>
			</tt>.<br/>
		</xsl:if>
		<xsl:if test="string-length(@mult)!=0">Multiple of: <tt>
				<xsl:value-of select="@mult"/>
			</tt>.<br/>
		</xsl:if>
		<xsl:if test="@align!='0'">Must be aligned on <xsl:if test="@align='pow2'">an appropriate power of 2</xsl:if>
			<xsl:if test="@align!='pow2'">
				<xsl:value-of select="@align"/>-byte </xsl:if> address boundary.<br/>
		</xsl:if>
		<xsl:apply-templates/>
		<xsl:text>"</xsl:text><xsl:value-of select="$nl"/>
	</xsl:template>
	<xsl:template match="precondition" mode="normalize">
		<xsl:text>	"Precondition", "``</xsl:text>
		<xsl:value-of select="." mode="normalize"/>
		<xsl:text>``"</xsl:text><xsl:value-of select="$nl"/>
	</xsl:template>
	<xsl:template match="postcondition" mode="normalize">
		<xsl:text>	"Postcondition", "``</xsl:text>
		<xsl:value-of select="." mode="normalize"/>
		<xsl:text>``"</xsl:text><xsl:value-of select="$nl"/>
	</xsl:template>

	<xsl:template name="indentinner">
	 	<xsl:param name="text"/>
	  	<xsl:choose>
		    	<xsl:when test="contains($text,'&#xA;') and normalize-space(substring-after($text,'&#xA;'))">
		      		<xsl:value-of select="substring-before($text,'&#xA;')"/>
		      		<xsl:text>&#xA;	</xsl:text>
		      		<xsl:call-template name="indentinner">
		        		<xsl:with-param name="text" select="substring-after($text,'&#xA;')"/>
		      		</xsl:call-template>
		    	</xsl:when>
		    	<xsl:when test="contains($text,'&#xA;')">
		      		<xsl:value-of select="substring-before($text,'&#xA;')"/>
		      		<xsl:text>&#xA;</xsl:text>
		    	</xsl:when>
		    	<xsl:otherwise>
		      		<xsl:value-of select="$text"/>
		    	</xsl:otherwise>
	  	</xsl:choose>
	</xsl:template>

	<xsl:template name="indent">
		<xsl:param name="text"/>
		<xsl:text>	</xsl:text>
		<xsl:call-template name="indentinner">
			<xsl:with-param name="text" select="$text"/>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="text()" mode="normalize">
		<xsl:if test="string-length(normalize-space(.))">
			<xsl:if test="not(normalize-space(substring(.,1,1))) and count(preceding-sibling::*)&gt;0 and not(name(preceding-sibling::*[1])='par' or name(preceding-sibling::*[1])='nl' or name(preceding-sibling::*[1])='enum' or name(preceding-sibling::*[1])='c')">
				<xsl:text> </xsl:text>
			</xsl:if>
			<xsl:value-of select="normalize-space(.)"/>
			<xsl:if test="not(normalize-space(substring(.,string-length(.),1))) and count(following-sibling::*)">
				<xsl:text> </xsl:text>
			</xsl:if>
		</xsl:if>
	</xsl:template>

	<xsl:template name="codeinner">
	 	<xsl:param name="text"/>
	  	<xsl:choose>
		    	<xsl:when test="contains($text,'&#xA;')">
			 	<xsl:value-of select="$nl"/>
			 	<xsl:value-of select="$nl"/>
				<xsl:text>.. code:: c</xsl:text><xsl:value-of select="$nl"/>
				<xsl:value-of select="$nl"/>
				<xsl:call-template name="indent">
					<xsl:with-param name="text" select="$text"/>
				</xsl:call-template>
				<xsl:value-of select="$nl"/>
				<xsl:value-of select="$nl"/>
		    	</xsl:when>
		    	<xsl:otherwise>
		      		<xsl:text>``</xsl:text><xsl:value-of select="$text"/><xsl:text>``</xsl:text>
		    	</xsl:otherwise>
	  	</xsl:choose>
	</xsl:template>

	<xsl:template name="code">
		<xsl:param name="content"/>
		<xsl:param name="prefix"/>
		<xsl:param name="suffix"/>
		<xsl:call-template name="codeinner">
			<xsl:with-param name="text">
				<xsl:value-of select="$prefix"/>
				<xsl:value-of select="$content"/>
				<xsl:value-of select="$suffix"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="code">
		<xsl:call-template name="codeinner">
			<xsl:with-param name="text">
				<xsl:apply-templates/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

	<xsl:template match="code" mode="normalize">
		<xsl:call-template name="codeinner">
			<xsl:with-param name="text">
				<xsl:apply-templates mode="normalize"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

</xsl:stylesheet>
