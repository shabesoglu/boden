#include <bdn/init.h>
#include <bdn/ScrollView.h>

#include <bdn/test.h>
#include <bdn/test/testView.h>
#include <bdn/test/MockScrollViewCore.h>
#include <bdn/test/MockButtonCore.h>

#include <bdn/Button.h>

using namespace bdn;

void testSizingWithContentView(P<bdn::test::ViewWithTestExtensions<ScrollView>> scrollView,
                               P<IBase> keepAliveInContinuations, P<bdn::test::MockUiProvider> uiProvider,
                               std::function<Size()> getSizeFunc)
{
    // we add a button as a content view
    P<Button> button = newObj<Button>();
    button->setLabel("HelloWorld");

    Margin buttonMargin;

    SECTION("noMargin")
    {
        // do nothing
    }

    SECTION("semMargin")
    {
        button->setMargin(UiMargin(UiLength::sem(1), UiLength::sem(2), UiLength::sem(3), UiLength::sem(4)));

        // 1 sem = 20 DIPs in our mock ui
        buttonMargin = Margin(20, 40, 60, 80);
    }

    SECTION("dipMargin")
    {
        button->setMargin(UiMargin(1, 2, 3, 4));

        buttonMargin = Margin(1, 2, 3, 4);
    }

    scrollView->setContentView(button);

    P<bdn::test::MockButtonCore> buttonCore = cast<bdn::test::MockButtonCore>(button->getViewCore());

    // Sanity check. Verify the fake button size. 9.75 , 19.60 per character,
    // rounded up to 1/3 pixel size, plus 10x8 for border
    Size buttonSize(std::ceil(10 * 9.75 * 3) / 3 + 10, 19 + 2.0 / 3 + 8);
    REQUIRE_ALMOST_EQUAL(buttonCore->calcPreferredSize(), buttonSize, Size(0.0000001, 0.0000001));

    Size expectedSize = buttonSize + buttonMargin;

    // the sizing info will update asynchronously. So we need to do the
    // check async as well.
    CONTINUE_SECTION_WHEN_IDLE(getSizeFunc, expectedSize, keepAliveInContinuations)
    {
        Size size = getSizeFunc();

        REQUIRE_ALMOST_EQUAL(size, expectedSize, Size(0.0000001, 0.0000001));
    };
}

TEST_CASE("ScrollView", "[ui]")
{
    SECTION("View-base")
    bdn::test::testView<ScrollView>();

    SECTION("ScrollView-specific")
    {
        P<bdn::test::ViewTestPreparer<ScrollView>> preparer = newObj<bdn::test::ViewTestPreparer<ScrollView>>();

        P<bdn::test::ViewWithTestExtensions<ScrollView>> scrollView = preparer->createView();

        P<bdn::test::MockScrollViewCore> core = cast<bdn::test::MockScrollViewCore>(scrollView->getViewCore());
        REQUIRE(core != nullptr);

        // continue testing after the async init has finished
        CONTINUE_SECTION_WHEN_IDLE(preparer, scrollView, core){
            // testView already tests the initialization of properties defined
            // in View. So we only have to test the Window-specific things here.
            SECTION("constructWindowSpecific"){}

            SECTION("changeWindowProperty"){
                SECTION("contentView"){SECTION("!=null"){P<Button> button = newObj<Button>();
        bdn::test::_testViewOp(
            scrollView, preparer, [scrollView, button, preparer]() { scrollView->setContentView(button); },
            [scrollView, button, preparer] { REQUIRE(scrollView->getContentView() == cast<View>(button)); },
            bdn::test::ExpectedSideEffect_::invalidateSizingInfo         // should have caused sizing info to be
                                                                         // invalidated
                | bdn::test::ExpectedSideEffect_::invalidateParentLayout // should cause a parent layout
                                                                         // update since sizing info was
                                                                         // invalidated
                | bdn::test::ExpectedSideEffect_::invalidateLayout       // should have caused a layout invalidation
        );
    }

    SECTION("null (was already null)")
    {
        // check if the intended precondition for the test is actually true
        REQUIRE(scrollView->getContentView() == nullptr);

        // basically we only test here that there is no crash when the content
        // view is set to null and that it does not result in a sizing info
        // update.
        bdn::test::_testViewOp(scrollView, preparer, [scrollView]() { scrollView->setContentView(nullptr); },
                               [scrollView] { REQUIRE(scrollView->getContentView() == nullptr); },
                               0 // should not have caused a sizing info update (since there was no
                                 // change) should not have caused parent layout update
        );
    }

    SECTION("null (was not null)")
    {
        P<Button> button = newObj<Button>();
        scrollView->setContentView(button);

        // basically we only test here that there is no crash when the content
        // view is set to null and that it does not result in a sizing info
        // update.
        bdn::test::_testViewOp(
            scrollView, preparer, [scrollView]() { scrollView->setContentView(nullptr); },
            [scrollView]() { REQUIRE(scrollView->getContentView() == nullptr); },
            bdn::test::ExpectedSideEffect_::invalidateSizingInfo         // should have caused sizing info to be
                                                                         // invalidated
                | bdn::test::ExpectedSideEffect_::invalidateParentLayout // should cause a parent layout
                                                                         // update since sizing info was
                                                                         // invalidated
                | bdn::test::ExpectedSideEffect_::invalidateLayout       // should have caused a layout invalidation
        );
    }
}
}

SECTION("contentViewParent")
{
    P<Button> child = newObj<Button>();

    SECTION("parent is set directly after add")
    {
        scrollView->setContentView(child);

        BDN_REQUIRE(child->getParentView() == cast<View>(scrollView));
    }

    SECTION("null after destroy")
    {
        {
            bdn::test::ViewTestPreparer<ScrollView> preparer2;

            P<bdn::test::ViewWithTestExtensions<ScrollView>> scrollView2 = preparer2.createView();

            scrollView2->setContentView(child);
        }

        // preparer2 is now gone, so the view is not referenced there anymore.
        // But there may still be a scheduled sizing info update pending that
        // holds a reference to the window. Since we want the view to be
        // destroyed, we do the remaining test asynchronously after all pending
        // operations are done.

        CONTINUE_SECTION_WHEN_IDLE_WITH([child]() { BDN_REQUIRE(child->getParentView() == nullptr); });
    }
}

SECTION("sizing")
{
    SECTION("no contentView")
    {
        SECTION("calcPreferredSize")
        REQUIRE(scrollView->calcPreferredSize() == Size(0, 0));
    }

    SECTION("with contentView")
    {
        SECTION("calcPreferredSize")
        testSizingWithContentView(scrollView, preparer, preparer->getUiProvider(),
                                  [scrollView]() { return scrollView->calcPreferredSize(); });
    }
}

SECTION("contentView aligned on full pixels")
{
    P<Button> child = newObj<Button>();
    child->setLabel("hello");

    SECTION("weird child margin")
    child->setMargin(UiMargin(0.12345678));

    SECTION("weird window padding")
    scrollView->setPadding(UiMargin(0.12345678));

    scrollView->setContentView(child);

    CONTINUE_SECTION_WHEN_IDLE(child, scrollView)
    {
        // the mock views we use have 3 pixels per dip
        double pixelsPerDip = 3;

        Point pos = child->position();

        REQUIRE_ALMOST_EQUAL(pos.x * pixelsPerDip, std::round(pos.x * pixelsPerDip), 0.000001);
        REQUIRE_ALMOST_EQUAL(pos.y * pixelsPerDip, std::round(pos.y * pixelsPerDip), 0.000001);

        Size size = child->size();
        REQUIRE_ALMOST_EQUAL(size.width * pixelsPerDip, std::round(size.width * pixelsPerDip), 0.000001);
        REQUIRE_ALMOST_EQUAL(size.height * pixelsPerDip, std::round(size.height * pixelsPerDip), 0.000001);
    };
}

SECTION("getChildList")
{
    SECTION("empty")
    {
        List<P<View>> childList;
        scrollView->getChildViews(childList);

        REQUIRE(childList.empty());
    }

    SECTION("non-empty")
    {
        P<Button> child = newObj<Button>();
        scrollView->setContentView(child);

        List<P<View>> childList;
        scrollView->getChildViews(childList);

        REQUIRE(childList.size() == 1);
        REQUIRE(childList.front() == cast<View>(child));
    }
}

SECTION("removeAllChildViews")
{
    SECTION("no content view")
    {
        scrollView->removeAllChildViews();

        List<P<View>> childList;
        scrollView->getChildViews(childList);

        REQUIRE(childList.empty());
    }

    SECTION("with content view")
    {
        P<Button> child = newObj<Button>();
        scrollView->setContentView(child);

        scrollView->removeAllChildViews();

        REQUIRE(scrollView->getContentView() == nullptr);
        REQUIRE(child->getParentView() == nullptr);

        List<P<View>> childList;
        scrollView->getChildViews(childList);

        REQUIRE(childList.empty());
    }
}

SECTION("content view detached before destruction begins")
{
    P<Button> child = newObj<Button>();
    scrollView->setContentView(child);

    struct LocalTestData_ : public Base
    {
        bool destructorRun = false;
        int childParentStillSet = -1;
        int childStillChild = -1;
    };

    P<LocalTestData_> data = newObj<LocalTestData_>();

    scrollView->setDestructFunc([data, child](bdn::test::ViewWithTestExtensions<ScrollView> *win) {
        data->destructorRun = true;
        data->childParentStillSet = (child->getParentView() != nullptr) ? 1 : 0;
        data->childStillChild = (win->getContentView() != nullptr) ? 1 : 0;
    });

    BDN_CONTINUE_SECTION_WHEN_IDLE(data, child)
    {
        // All test objects should have been destroyed by now.
        // First verify that the destructor was even called.
        REQUIRE(data->destructorRun);

        // now verify what we actually want to test: that the
        // content view's parent was set to null before the destructor
        // of the parent was called.
        REQUIRE(data->childParentStillSet == 0);

        // the child should also not be a child of the parent
        // from the parent's perspective anymore.
        REQUIRE(data->childStillChild == 0);
    };
}
}
;
}
}
