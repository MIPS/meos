<?xml version="1.0" encoding="UTF-8"?>
	<!--
		XSL stylesheet to convert XHTML subset to xsl:fo for printable/PDF
		layout
	-->
<!DOCTYPE xsl:stylesheet [
 <!ENTITY nbsp "&#xa0;">
]>
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:x="http://www.w3.org/1999/xhtml">
	<xsl:output method="xml" version="1.0" encoding="UTF-8"
		standalone="no" omit-xml-declaration="no" indent="yes" media-type="application/xml" />

	<xsl:param name="sub-title" select="'Secondary title'"/>
	<xsl:param name="revision" select="'X.X'"/>
	<xsl:param name="main-title" select="'Main Title'"/>
	<xsl:param name="issue-date" select="'Issue Date'"/>
	<xsl:param name="file-name" select="'filename.pdf'"/>
	<xsl:param name="author" select="'MIPS Tech, LLC'" />

	<xsl:variable name="alphabet" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

	<xsl:variable name="confidentiality-summary">
		Confidential
	</xsl:variable>
	<xsl:variable name="copyright-notice">
		Copyright © MIPS Tech, LLC. All Rights Reserved.
	</xsl:variable>

	<xsl:template match="x:html">
		<fo:root xmlns:fo="http://www.w3.org/1999/XSL/Format">
			<fo:layout-master-set>
				<fo:simple-page-master master-name="A4even"
					page-height="297mm" page-width="210mm" margin-top="10mm"
					margin-bottom="10mm" margin-left="25mm" margin-right="25mm">
					<fo:region-body margin-top="12mm" margin-bottom="10mm" />
					<fo:region-before extent="8mm" region-name="header-even" />
					<fo:region-after extent="8mm" region-name="footer-even" />
				</fo:simple-page-master>
				<fo:simple-page-master master-name="A4odd"
					page-height="297mm" page-width="210mm" margin-top="12mm"
					margin-bottom="10mm" margin-left="25mm" margin-right="25mm">
					<fo:region-body margin-top="10mm" margin-bottom="10mm" />
					<fo:region-before extent="8mm" region-name="header-odd" />
					<fo:region-after extent="8mm" region-name="footer-odd" />
				</fo:simple-page-master>
				<fo:page-sequence-master master-name="A4document">
					<fo:repeatable-page-master-alternatives>
						<fo:conditional-page-master-reference
							odd-or-even="even" master-reference="A4even" />
						<fo:conditional-page-master-reference
							odd-or-even="odd" master-reference="A4odd" />
					</fo:repeatable-page-master-alternatives>
				</fo:page-sequence-master>
			</fo:layout-master-set>
			<fo:declarations>
				<x:xmpmeta xmlns:x="adobe:ns:meta/">
					<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
						<rdf:Description rdf:about="" xmlns:dc="http://purl.org/dc/elements/1.1/">
							<!-- Dublin Core properties go here -->
							<dc:title><xsl:copy-of select="$main-title" /> - <xsl:copy-of select="$sub-title" /></dc:title>
							<dc:creator><xsl:copy-of select="$author" /></dc:creator>
							<dc:description><xsl:copy-of select="$main-title" /> - <xsl:copy-of select="$sub-title" /></dc:description>
						</rdf:Description>
					</rdf:RDF>
				</x:xmpmeta>
			</fo:declarations>
			<!-- PDF Bookmarks -->
			<fo:bookmark-tree>
				<fo:bookmark internal-destination="table-of-contents">
					<fo:bookmark-title>Table of Contents</fo:bookmark-title>
				</fo:bookmark>
				<xsl:apply-templates select="//x:h1" mode="bookmarks" />
			</fo:bookmark-tree>
			<!-- Generate the document content -->
			<fo:page-sequence master-reference="A4document"
				font-size="10pt">
				<xsl:apply-templates select="x:head">
				</xsl:apply-templates>
				<xsl:apply-templates select="x:body" />
			</fo:page-sequence>
		</fo:root>
	</xsl:template>

	<xsl:template match="x:head">
		<fo:static-content flow-name="header-even">
			<fo:block border-after-style="solid">
				<fo:table table-layout="fixed"
					width="100%">
					<fo:table-body>
						<fo:table-row>
							<fo:table-cell display-align="after">
								<fo:block text-align="left">MIPS Tech, LLC
								</fo:block>
							</fo:table-cell>
							<fo:table-cell display-align="after">
								<fo:block text-align="center">
									<xsl:copy-of select="$confidentiality-summary" />
								</fo:block>
							</fo:table-cell>
							<fo:table-cell display-align="after">
								<fo:block text-align="right">
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</fo:block>
		</fo:static-content>

		<fo:static-content flow-name="footer-even">
			<fo:block border-before-style="solid">
				<fo:table table-layout="fixed"
					width="100%">
					<fo:table-body>
						<fo:table-row>
							<fo:table-cell>
								<fo:block text-align="left">
									<xsl:copy-of select="$revision" />
								</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block text-align="center">
									<fo:page-number />
								</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block text-align="right">
									<xsl:copy-of select="$sub-title" />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</fo:block>
		</fo:static-content>

		<fo:static-content flow-name="header-odd">
			<fo:block border-after-style="solid">
				<fo:table table-layout="fixed"
					width="100%">
					<fo:table-body>
						<fo:table-row>
							<fo:table-cell display-align="after">
								<fo:block text-align="left">
								</fo:block>
							</fo:table-cell>
							<fo:table-cell display-align="after">
								<fo:block text-align="center">
									<xsl:copy-of select="$confidentiality-summary" />
								</fo:block>
							</fo:table-cell>
							<fo:table-cell display-align="after">
								<fo:block text-align="right">MIPS Tech, LLC
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</fo:block>
		</fo:static-content>

		<fo:static-content flow-name="footer-odd">
			<fo:block border-before-style="solid">
				<fo:table table-layout="fixed"
					width="100%">
					<fo:table-body>
						<fo:table-row>
							<fo:table-cell>
								<fo:block text-align="left">
									<xsl:copy-of select="$main-title" />
								</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block text-align="center">
									<fo:page-number />
								</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block text-align="right">
									<xsl:copy-of select="$revision" />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</fo:block>
		</fo:static-content>
	</xsl:template>

	<xsl:template match="x:body">
		<!-- Title page stuff -->
		<fo:flow flow-name="xsl-region-body">
			<fo:block font-size="18pt" font-weight="bold" text-align="center"
				padding-before="50mm">
				<xsl:copy-of select="$main-title" />
			</fo:block>
			<fo:block font-size="18pt" font-weight="bold" text-align="center"
				space-before="8mm">
				<xsl:copy-of select="$sub-title" />
			</fo:block>
			<fo:block text-align="center" space-before="40mm">
				<xsl:copy-of select="$copyright-notice" />
			</fo:block>
			<fo:block space-before="30mm">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="15%" />
					<fo:table-column column-width="10%" />
					<fo:table-column column-width="75%" />
					<fo:table-body>
						<fo:table-row height="8mm">
							<fo:table-cell>
								<fo:block>
									Filename
							</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>:</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>
									<xsl:copy-of select="$file-name" />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
						<fo:table-row height="8mm">
							<fo:table-cell>
								<fo:block>Version</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>:</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>
									<xsl:copy-of select="$revision" />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
						<fo:table-row height="8mm">
							<fo:table-cell>
								<fo:block>Issue Date</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>:</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>
									<xsl:copy-of select="$issue-date" />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
						<fo:table-row height="8mm">
							<fo:table-cell>
								<fo:block>Author</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>:</fo:block>
							</fo:table-cell>
							<fo:table-cell>
								<fo:block>
									<xsl:copy-of select="$author" />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</fo:block>
			<!-- End of title page stuff -->
			<!--  Table of contents -->
			<fo:block keep-with-next="always" font-size="16pt"
				font-weight="bold" space-before="16pt" space-after="5pt"
				break-before="page" id="table-of-contents">
				Contents
			</fo:block>
			<xsl:apply-templates select="*" mode="toc" />
			<!-- End of TOC -->
			<!-- Document body -->
			<xsl:apply-templates />
			<!-- End of document body -->
		</fo:flow>
	</xsl:template>

	<xsl:template match="x:p">
		<!-- XHTML paragraph template -->
		<xsl:choose>
			<xsl:when test="contains(@class, 'live') or contains(@class, 'section-links')">
				<!-- Skip section links and live content - only used in browser HTML -->
			</xsl:when>
			<xsl:otherwise>
				<!-- Ordinary paragraphs -->
				<fo:block space-before="4pt" space-after="4pt">
					<xsl:apply-templates />
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:pre">
		<!-- XHTML pre template -->
		<xsl:choose>
			<xsl:when test="count(ancestor::x:table)">
				<fo:block space-before="4pt" space-after="4pt" white-space="pre" wrap-option="wrap" border-style="none" border-width="0.75pt"  page-break-inside="avoid" font-family="courier,monospace"><xsl:value-of select="substring(concat('',.),2)"/></fo:block>
			</xsl:when>
			<xsl:otherwise>
				<fo:block space-before="4pt" space-after="4pt" white-space="pre" wrap-option="wrap" border-style="solid" border-width="0.75pt"  page-break-inside="avoid" font-family="courier,monospace"><xsl:value-of select="substring(concat('',.),2)"/></fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:blockquote">
		<!-- We use XHTML blockquotes for indented paragraphs  -->
		<fo:block start-indent="12mm" space-before="12pt"
			space-after="12pt">
			<xsl:apply-templates />
		</fo:block>
	</xsl:template>

	<xsl:template match="x:h1">
		<!-- Header level 1 -->
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!-- Function descriptions aren't numbered -->
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="16pt" font-weight="bold" space-before="16pt"
					space-after="5pt" break-before="page">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:apply-templates />
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="16pt" font-weight="bold" space-before="16pt"
					space-after="5pt" break-before="page">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1, 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1"/>
							</xsl:otherwise>
						</xsl:choose>.<fo:character character=' '/>
						<xsl:apply-templates />
					</fo:basic-link>
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h1" mode="toc">
		<!-- Header level 1 in TOC -->
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!-- Function descriptions aren't numbered -->
				<fo:block id="toc-{generate-id()}" font-weight="bold"
					space-before="8pt" space-after="5pt" text-align-last="justify">
					<fo:basic-link internal-destination="{generate-id()}">
						<xsl:apply-templates mode="normalise"/>
						<fo:leader leader-pattern="dots" />
						<fo:page-number-citation ref-id="{generate-id()}" />
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:block id="toc-{generate-id()}" keep-with-next="always"
					font-weight="bold" space-before="8pt" space-after="5pt"
					text-align-last="justify">
					<fo:basic-link internal-destination="{generate-id()}">
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1, 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1"/>
							</xsl:otherwise>
						</xsl:choose>.<fo:character character=' '/>
						<xsl:apply-templates mode="normalise"/>
						<fo:leader leader-pattern="dots" />
						<fo:page-number-citation ref-id="{generate-id()}" />
					</fo:basic-link>
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h1" mode="bookmarks">
		<!-- Header level 1 in index -->
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!-- Function descriptions aren't numbered -->
				<fo:bookmark internal-destination="{generate-id()}"
							starting-state="hide">
					<fo:bookmark-title>
						<xsl:apply-templates mode="normalise"/>
						</fo:bookmark-title>
				</fo:bookmark>
			</xsl:when>
			<xsl:otherwise>
				<fo:bookmark internal-destination="{generate-id()}" starting-state="hide">
					<fo:bookmark-title>
						<xsl:choose>
							<xsl:when test="text()='Appendices'">A</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1, 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1"/>
							</xsl:otherwise>
						</xsl:choose>. <xsl:apply-templates mode="normalise"/>
					</fo:bookmark-title>
					<xsl:variable name="index" select="count(preceding::x:h1)+1"/>
					<xsl:apply-templates select="following::x:h2[count(preceding::x:h1)=$index]" mode="bookmarks"/>
				</fo:bookmark>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>


	<xsl:template match="x:h2">
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!-- Function headers aren't numbered - but they do start a new page -->
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="14pt" font-weight="bold" space-before="14pt"
					space-after="5pt" break-before="page">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:apply-templates/>
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:when test="text()='Appendices' or preceding::x:h1[text()='Appendices']">
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="14pt" font-weight="bold" space-before="14pt"
					space-after="5pt" break-before="page">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/><fo:character character=' '/>
						<xsl:apply-templates mode="normalise"/>
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="14pt" font-weight="bold" space-before="14pt"
					space-after="5pt">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/><fo:character character=' '/>
						<xsl:apply-templates mode="normalise"/>
					</fo:basic-link>
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h2" mode="toc">
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'description') or contains(../@class,'description')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!-- Function headers aren't numbered  -->
				<fo:block id="toc-{generate-id()}" margin-left="20pt"
					text-align-last="justify">
					<fo:basic-link internal-destination="{generate-id()}">
						<xsl:apply-templates/>
						<fo:leader leader-pattern="dots" />
						<fo:page-number-citation ref-id="{generate-id()}" />
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:block id="toc-{generate-id()}" margin-left="20pt"
					text-align-last="justify">
					<fo:basic-link internal-destination="{generate-id()}">
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/><fo:character character=' '/>
						<xsl:apply-templates mode="normalise"/>
						<fo:leader leader-pattern="dots" />
						<fo:page-number-citation ref-id="{generate-id()}" />
					</fo:basic-link>
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h2" mode="bookmarks">
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'description') or contains(../@class,'description')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!-- Function headers aren't numbered  -->
				<fo:bookmark internal-destination="{generate-id()}" starting-state="hide">
					<fo:bookmark-title>
						<xsl:apply-templates mode="normalise"/>
					</fo:bookmark-title>
				</fo:bookmark>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:bookmark internal-destination="{generate-id()}" starting-state="hide">
					<fo:bookmark-title>
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/>. <xsl:apply-templates mode="normalise"/>
					</fo:bookmark-title>
					<xsl:variable name="index" select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/>
					<xsl:apply-templates select="following::x:h3[count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])=$index]" mode="bookmarks"/>
				</fo:bookmark>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h3">
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<!--
					Function headers aren't numbered but they do start a new page and
					are in larger type
				-->
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="14pt" font-weight="bold" space-before="12pt"
					space-after="12pt" break-before="page">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:apply-templates/>
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:block id="{generate-id()}" keep-with-next="always"
					font-size="12pt" font-weight="bold" space-before="12pt"
					space-after="5pt">
					<fo:basic-link internal-destination="toc-{generate-id()}">
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])"/>.<xsl:value-of select="count(preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))]/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/><fo:character character=' '/>
						<xsl:apply-templates/>
					</fo:basic-link>
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h3" mode="toc">
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<fo:block id="toc-{generate-id()}" margin-left="40pt"
					text-align-last="justify">
					<fo:basic-link internal-destination="{generate-id()}">
						<xsl:apply-templates mode="normalise"/>
						<fo:leader leader-pattern="dots" />
						<fo:page-number-citation ref-id="{generate-id()}" />
					</fo:basic-link>
				</fo:block>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:block id="toc-{generate-id()}" margin-left="40pt"
					text-align-last="justify">
					<fo:basic-link internal-destination="{generate-id()}">
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])"/>.<xsl:value-of select="count(preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))]/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/><fo:character character=' '/>
						<xsl:apply-templates mode="normalise"/>
						<fo:leader leader-pattern="dots" />
						<fo:page-number-citation ref-id="{generate-id()}" />
					</fo:basic-link>
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h3" mode="bookmarks">
		<xsl:choose>
			<xsl:when test="contains(@class,'live') or contains(../@class,'live')"/>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<fo:bookmark internal-destination="{generate-id()}" starting-state="hide">
					<fo:bookmark-title>
						<xsl:apply-templates mode="normalise"/>
					</fo:bookmark-title>
				</fo:bookmark>
			</xsl:when>
			<xsl:otherwise>
				<!-- Standard version -->
				<fo:bookmark internal-destination="{generate-id()}" starting-state="hide">
					<fo:bookmark-title>
						<xsl:choose>
							<xsl:when test="text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count(preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])"/>.<xsl:value-of select="count(preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])-count(preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))]/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition') or contains(@class,'live') or contains(../@class,'live') or contains(@class,'description') or contains(../@class,'description'))])+1"/>. <xsl:apply-templates mode="normalise"/>
					</fo:bookmark-title>
				</fo:bookmark>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h4">
		<!--
			h4 and h5 have special variants for function definitions - to give
			the "black stripe" format
		-->
		<xsl:choose>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<fo:table width="100%" space-before="12pt" space-after="12pt"
					table-layout="fixed">
					<fo:table-body>
						<fo:table-row>
							<fo:table-cell color="white" background-color="black"
								font-weight="bold" padding-top="2pt" padding-bottom="2pt"
								padding-left="4pt" keep-with-next="always">
								<fo:block>
									<xsl:apply-templates />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</xsl:when>
			<xsl:otherwise>
				<fo:block keep-with-next="always" font-weight="bold"
					space-before="8pt" space-after="5pt">
					<xsl:apply-templates />
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:h5">
		<!--
			h4 and h5 have special variants for function definitions - to give
			the "black stripe" format
		-->
		<xsl:choose>
			<xsl:when test="contains(@class,'function-definition') or contains(../@class,'function-definition')">
				<fo:table width="100%" space-before="12pt" space-after="12pt"
					table-layout="fixed">
					<fo:table-body>
						<fo:table-row>
							<fo:table-cell color="white" background-color="black"
								font-weight="bold" padding-top="2pt" padding-bottom="2pt"
								padding-left="4pt" keep-with-next="always">
								<fo:block>
									<xsl:apply-templates />
								</fo:block>
							</fo:table-cell>
						</fo:table-row>
					</fo:table-body>
				</fo:table>
			</xsl:when>
			<xsl:otherwise>
				<fo:block keep-with-next="always" font-weight="bold"
					space-before="8pt">
					<xsl:apply-templates />
				</fo:block>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:em">
		<fo:inline font-style="italic">
			<xsl:apply-templates />
		</fo:inline>
	</xsl:template>

	<xsl:template match="x:code">
		<fo:inline font-family="courier,monospace">
			<xsl:apply-templates />
		</fo:inline>
	</xsl:template>

	<xsl:template match="x:tt">
		<fo:inline font-family="courier,monospace">
			<xsl:apply-templates />
		</fo:inline>
	</xsl:template>

	<xsl:template match="x:table">
		<xsl:choose>
			<!-- Class specific table layouts -->
			<xsl:when test="contains(@class,'live')"/>
			<xsl:when test="contains(@class,'function-arguments')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="20%" />
					<fo:table-column column-width="10%" />
					<fo:table-column column-width="35%" />
					<fo:table-column column-width="35%" />
					<xsl:apply-templates mode="function" />
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'function-sources')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="30%" />
					<fo:table-column column-width="70%" />
					<xsl:apply-templates mode="sources" />
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'function-limitations')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="30%" />
					<fo:table-column column-width="70%" />
					<xsl:apply-templates mode="function" />
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'definitions2')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="30%" />
					<fo:table-column column-width="70%" />
					<xsl:apply-templates />
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'data-definitions')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="20%" />
					<fo:table-column column-width="20%" />
					<fo:table-column column-width="10%" />
					<fo:table-column column-width="40%" />
					<xsl:apply-templates />
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'meos-arguments')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="33%" />
					<fo:table-column column-width="66%" />
					<xsl:apply-templates mode="function"/>
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'meos-limits')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="33%" />
					<fo:table-column column-width="66%" />
					<xsl:apply-templates mode="function"/>
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'meos-enums')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="33%" />
					<fo:table-column column-width="66%" />
					<xsl:apply-templates mode="function"/>
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'meos-typedefs')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="50%" />
					<fo:table-column column-width="50%" />
					<xsl:apply-templates mode="meos-typedefs"/>
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'meos-variables')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="50%" />
					<fo:table-column column-width="50%" />
					<xsl:apply-templates mode="function"/>
				</fo:table>
			</xsl:when>
			<xsl:when test="contains(@class,'meos-defines')">
				<fo:table table-layout="fixed" width="100%">
					<fo:table-column column-width="50%" />
					<fo:table-column column-width="50%" />
					<xsl:apply-templates mode="function"/>
				</fo:table>
			</xsl:when>
			<xsl:otherwise>
				<fo:table>
					<xsl:apply-templates />
				</fo:table>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:thead">
		<fo:table-header font-weight="bold">
			<xsl:apply-templates />
		</fo:table-header>
	</xsl:template>

	<xsl:template match="x:thead" mode="function">
		<!-- In function argument tables, the header is simply omitted -->
	</xsl:template>

	<xsl:template match="x:thead" mode="sources">
		<!-- In source file tables, the header is simply omitted -->
	</xsl:template>

	<xsl:template match="x:tbody" mode="function">
		<fo:table-body>
			<xsl:apply-templates mode="function" />
		</fo:table-body>
	</xsl:template>

	<xsl:template match="x:tbody" mode="sources">
		<fo:table-body>
			<xsl:apply-templates mode="sources" />
		</fo:table-body>
	</xsl:template>

	<xsl:template match="x:tbody">
		<fo:table-body>
			<xsl:apply-templates />
		</fo:table-body>
	</xsl:template>

	<xsl:template match="x:tbody" mode="meos-typedefs">
		<fo:table-body>
			<xsl:apply-templates mode="meos-typedefs"/>
		</fo:table-body>
	</xsl:template>

	<xsl:template match="x:tr" mode="meos-typedefs">
		<xsl:if test="x:td[2]!='Private.'">
			<fo:table-row>
				<xsl:apply-templates select="x:td[1]" mode="function"/>
				<xsl:apply-templates select="x:td[3]" mode="function"/>
			</fo:table-row>
		</xsl:if>
	</xsl:template>

	<xsl:template match="x:tr">
		<fo:table-row>
			<xsl:apply-templates />
		</fo:table-row>
	</xsl:template>
	<xsl:template match="x:tr" mode="function">
		<fo:table-row>
			<xsl:apply-templates mode="function" />
		</fo:table-row>
	</xsl:template>

	<xsl:template match="x:tr" mode="sources">
		<fo:table-row>
			<xsl:apply-templates mode="sources" />
		</fo:table-row>
	</xsl:template>

	<xsl:template match="x:th">
		<fo:table-cell border="1pt solid" padding="2pt" color="white"
			background-color="black">
			<fo:block start-indent="0pt">
				<xsl:apply-templates />
			</fo:block>
		</fo:table-cell>
	</xsl:template>

	<xsl:template match="x:td">
		<fo:table-cell border="1pt solid" padding="2pt">
			<fo:block start-indent="0pt">
				<xsl:apply-templates />
			</fo:block>
		</fo:table-cell>
	</xsl:template>

	<xsl:template match="x:td" mode="function">
		<fo:table-cell border="1pt solid" padding="2pt"
			border-color="rgb(200,200,200)">
			<fo:block start-indent="0pt">
				<xsl:apply-templates />
			</fo:block>
		</fo:table-cell>
	</xsl:template>

	<xsl:template match="x:td" mode="sources">
		<fo:table-cell border="1pt solid" padding="2pt"
			border-color="rgb(200,200,200)">
			<fo:block start-indent="0pt">
				<xsl:apply-templates mode="sources" />
			</fo:block>
		</fo:table-cell>
	</xsl:template>

	<xsl:template match="x:a" mode="sources">
		<fo:basic-link external-destination="url('{@href}')">
			<xsl:apply-templates />
		</fo:basic-link>
	</xsl:template>

	<xsl:template match="x:hr">
		<!--
			Do nothing - the printable document uses breaks and banner headers
			for visual division of the content.
		-->
	</xsl:template>

	<xsl:template match="x:div">
		<xsl:choose>
			<xsl:when test="contains(@class,'description')">
				<!--
					No heading
				-->
				<xsl:apply-templates select="x:p"/>
			</xsl:when>
			<xsl:when test="not(contains(@class,'live'))">

				<!--
					The <div> structure is used to capture (sub)section nesting. It has
					no direct effect on layout.
				-->
				<xsl:apply-templates />
			</xsl:when>
			<xsl:otherwise>
				<xsl:apply-templates select="x:div" />
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="x:ol|x:ul">
		   <fo:list-block margin-left="0.63cm" provisional-distance-between-starts="0.63cm">
		   	<xsl:apply-templates/>
		   </fo:list-block>
	</xsl:template>

	<xsl:template match="x:ol/x:li">
		<fo:list-item>
			<fo:list-item-label end-indent="label-end()">
				<fo:block>
					<xsl:number format="1."/>
				</fo:block>
			</fo:list-item-label>
			<fo:list-item-body start-indent="body-start()">
				<fo:block>
					<xsl:apply-templates select="*|text()"/>
				</fo:block>
			</fo:list-item-body>
		</fo:list-item>
	</xsl:template>

	<xsl:template match="x:ul/x:li">
		<fo:list-item>
			<fo:list-item-label end-indent="label-end()">
				<fo:block>&#x2022;</fo:block>
			</fo:list-item-label>
			<fo:list-item-body start-indent="body-start()">
				<fo:block>
					<xsl:apply-templates select="*|text()"/>
				</fo:block>
			</fo:list-item-body>
		</fo:list-item>
	</xsl:template>


	<xsl:template match="x:dt">
		<fo:block font-weight="bold" keep-with-next="always">
			<xsl:apply-templates select="*|text()"/>
		</fo:block>
	</xsl:template>
	<xsl:template match="x:dd">
		<fo:block start-indent=".63cm">
			<xsl:apply-templates select="*|text()"/>
		</fo:block>
	</xsl:template>

	<xsl:template match="x:a">
		<!--
			Apart from source files, hyperlinks in the HTML are ignored - the
			printable/PDF document has a table of contents rather than the
			navigation aids of the browseable document
		-->
		<xsl:variable name="href" select="substring(@href,2)"/>
		<xsl:variable name="ph1" select="//x:div[@id=$href]/x:h1"/>
		<xsl:variable name="ph2" select="//x:div[@id=$href]/x:h2"/>
		<xsl:variable name="ph3" select="//x:div[@id=$href]/x:h3"/>
                <xsl:variable name="ph4" select="//x:div[@id=$href]/x:h4"/>
		<xsl:choose>
			<xsl:when test="substring(@href,1,1) = '#'">
				Section <xsl:choose>
					<xsl:when test='$ph1'>
						<xsl:choose>
							<xsl:when test="$ph1/text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="$ph1/preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count($ph1/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph1/preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1, 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count($ph1/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>
					</xsl:when>
					<xsl:when test='$ph2'>
						<xsl:choose>
							<xsl:when test="$ph2/text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="$ph2/preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count($ph2/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph2/preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count($ph2/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count($ph2/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph2/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1"/>
					</xsl:when>
					<xsl:when test='$ph3'>
						<xsl:choose>
							<xsl:when test="$ph3/text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="$ph3/preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count($ph3/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph3/preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count($ph3/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count($ph3/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph3/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>.<xsl:value-of select="count($ph3/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph3/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1"/>
					</xsl:when>
					<xsl:when test='$ph4'>
						<xsl:choose>
							<xsl:when test="$ph4/text()='Appendices'">
								A
							</xsl:when>
							<xsl:when test="$ph4/preceding::x:h1[text()='Appendices']">
								<xsl:value-of select="substring($alphabet, count($ph4/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph4/preceding::x:h1[text()='Appendices']/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]), 1)"/>
							</xsl:when>
							<xsl:otherwise>
								<xsl:value-of select="count($ph4/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>
							</xsl:otherwise>
						</xsl:choose>.<xsl:value-of select="count($ph4/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph4/preceding::x:h1[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])"/>.<xsl:value-of select="count($ph4/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])-count($ph4/preceding::x:h2[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))]/preceding::x:h3[not(contains(@class,'function-definition') or contains(../@class,'function-definition'))])+1"/>: "<xsl:value-of select="$ph4"/>"
					</xsl:when>
					<xsl:otherwise>
						BAD LINK
					</xsl:otherwise>
				</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<xsl:apply-templates />
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="*" mode="toc"><xsl:apply-templates select="*" mode="toc"/></xsl:template>
	<xsl:template match="*" mode="bookmarks"><xsl:apply-templates select="*" mode="bookmarks"/></xsl:template>
	<xsl:template match="text()" mode="normalise"><xsl:value-of select="normalize-space(.)"/></xsl:template>

</xsl:stylesheet>
