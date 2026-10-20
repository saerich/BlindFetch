from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import roc_auc_score, accuracy_score, balanced_accuracy_score, classification_report, f1_score, recall_score, confusion_matrix
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

TRAIN_CSV = "Data/MergedExecutions.csv"
TEST_CSV = "Data/HeldOutExecutions.csv"

N_BOOTSTRAPS = 10000
RANDOM_STATE = 42

COLUMNS = ["round", "policy", "ordering", "S", "ttfb_ms", "ttlb_ms", "blocks_sent", "perm_id", "target_position", "Slot0", "Slot1", "Slot2", "Slot3"]

TIMING_FEATURES = ["ttfb_ms", "ttlb_ms", "blocks_sent", "duration"]

SLOT_COLS = ["Slot0", "Slot1", "Slot2", "Slot3"]
POLICIES = ["EarlyClose", "FixedBudget"]
ORDERINGS = ["Ordered", "Unordered"]


def load_data(path):
    data = pd.read_csv(path)

    if len(data.columns) != len(COLUMNS):
        raise ValueError(f"{path}: Expected {len(COLUMNS)} columns, found {len(data.columns)}")

    data.columns = COLUMNS

    for col in ["ttfb_ms", "ttlb_ms", "blocks_sent", "target_position"]:
        data[col] = pd.to_numeric(data[col], errors="raise")

    data["target_position"] = (data["target_position"].astype(int))

    data["duration"] = (data["ttlb_ms"] - data["ttfb_ms"])

    if data[TIMING_FEATURES + SLOT_COLS].isna().any().any():
        raise ValueError(f"{path}: Missing feature values")

    if not data["target_position"].between(0, 3).all():
        raise ValueError(f"{path}: Invalid target_position")

    if data["round"].isna().any():
        raise ValueError(f"{path}: Missing round identifiers")
    return data

def features(sub):
    return pd.get_dummies(sub[TIMING_FEATURES + SLOT_COLS], columns=SLOT_COLS, dtype=int)

def aligned_features(train, test):
    X_train = features(train)
    X_test = features(test)

    X_test = X_test.reindex(columns=X_train.columns, fill_value=0)
    return X_train, X_test


def firmware_labels(sub):
    positions = sub["target_position"].to_numpy()
    slots = sub[SLOT_COLS].to_numpy()

    return slots[np.arange(len(sub)), positions]


def map_slots_to_firmware(sub, predicted_slots):
    slots = sub[SLOT_COLS].to_numpy()
    predicted_slots = np.asarray(predicted_slots, dtype=int)
    return slots[np.arange(len(sub)), predicted_slots]

def bootstrap_ci(y, pred, prob, groups, labels):
    y = np.asarray(y)
    pred = np.asarray(pred)
    prob = np.asarray(prob)
    groups = np.asarray(groups)
    labels = np.asarray(labels)

    unique_groups = np.unique(groups)

    group_idx = {
        g: np.flatnonzero(groups == g)
        for g in unique_groups
    }

    rng = np.random.default_rng(RANDOM_STATE)

    boot_acc = []
    boot_f1 = []
    boot_auc = []

    for _ in range(N_BOOTSTRAPS):
        sampled_groups = rng.choice(
            unique_groups,
            size=len(unique_groups),
            replace=True
        )

        idx = np.concatenate([
            group_idx[g]
            for g in sampled_groups
        ])

        y_b = y[idx]
        pred_b = pred[idx]
        prob_b = prob[idx]

        boot_acc.append(
            accuracy_score(y_b, pred_b)
        )

        boot_f1.append(
            f1_score(
                y_b,
                pred_b,
                labels=labels,
                average="macro",
                zero_division=0
            )
        )

        try:
            if len(np.unique(y_b)) != len(labels):
                continue

            boot_auc.append(
                roc_auc_score(
                    y_b,
                    prob_b,
                    labels=labels,
                    multi_class="ovr",
                    average="macro"
                )
            )

        except ValueError:
            continue

    return {
        "accuracy_ci": np.percentile(
            boot_acc, [2.5, 97.5]
        ),
        "f1_ci": np.percentile(
            boot_f1, [2.5, 97.5]
        ),
        "auc_ci": (
            np.percentile(boot_auc, [2.5, 97.5])
            if boot_auc else np.array([np.nan, np.nan])
        )
    }

def evaluate_heldout(y_train, y_test, X_train, X_test, groups):
    y_train = np.asarray(y_train)
    y_test = np.asarray(y_test)

    model = RandomForestClassifier(n_estimators=100, random_state=RANDOM_STATE)
    # Fit exclusively on development data.
    model.fit(X_train, y_train)
    # Evaluate once on untouched held-out data.
    pred = model.predict(X_test)
    prob = model.predict_proba(X_test)

    labels = model.classes_

    if not set(np.unique(y_test)).issubset(set(labels)):
        raise ValueError("Held-out dataset contains a target class not present in the development dataset")

    if len(np.unique(y_test)) != len(labels):
        raise ValueError("Held-out dataset does not contain every training target class")

    ci = bootstrap_ci(y_test, pred, prob, groups, labels)

    return {
        "model": model,
        "pred": pred,
        "prob": prob,
        "labels": labels,
        "accuracy": accuracy_score(y_test, pred),
        "balanced_accuracy": balanced_accuracy_score(y_test, pred),
        "macro_f1": f1_score(y_test, pred, labels=labels, average="macro", zero_division=0),
        "macro_auc": roc_auc_score(y_test, prob, labels=labels, multi_class="ovr", average="macro"),
        "recall": recall_score(y_test, pred, labels=labels, average=None, zero_division=0),
        "cm": confusion_matrix(y_test, pred, labels=labels),
        **ci
    }

train_data = load_data(TRAIN_CSV)
test_data = load_data(TEST_CSV)

results = []

for policy in POLICIES:
    for ordering in ORDERINGS:
        train = train_data[(train_data.policy == policy) & (train_data.ordering == ordering)].copy().reset_index(drop=True)
        test = test_data[(test_data.policy == policy) & (test_data.ordering == ordering)].copy().reset_index(drop=True)

        if train.empty or test.empty:
            print(f"Skipping {policy} + {ordering}: missing development or held-out data")
            continue

        X_train, X_test = aligned_features(train, test)
        groups = test["round"]
        train_fw = firmware_labels(train)
        test_fw = firmware_labels(test)
        slot = evaluate_heldout(train["target_position"], test["target_position"], X_train, X_test, groups)
        mediated_fw = map_slots_to_firmware(test, slot["pred"])
        mediated_acc = accuracy_score(test_fw, mediated_fw)
        mediated_f1 = f1_score(test_fw, mediated_fw, average="macro", zero_division=0)
        direct = evaluate_heldout(train_fw, test_fw, X_train, X_test, groups)
        label = (policy.replace("EarlyClose", "Early Close").replace("FixedBudget", "Fixed Budget") + "\n" + ordering.replace("Unordered", "Randomized"))
        results.append((label, slot, mediated_acc, mediated_f1, direct))

        print("\n" + "=" * 90)
        print(f"{policy} + {ordering}")
        print("=" * 90)
        print(f"Development executions: {len(train)}")
        print(f"Held-out executions:    {len(test)}")
        print(f"Held-out rounds:        {groups.nunique()}")
        print("\nSLOT-MEDIATED ATTACK")
        print(f"Firmware accuracy: {mediated_acc:.4f}")
        print(f"Firmware macro-F1: {mediated_f1:.4f}")
        print(f"Slot macro-OvR AUROC: {slot['macro_auc']:.4f} [{slot['auc_ci'][0]:.4f}, {slot['auc_ci'][1]:.4f}]")
        print("\nDIRECT FIRMWARE ATTACK")
        print(f"Firmware accuracy: {direct['accuracy']:.4f} [{direct['accuracy_ci'][0]:.4f}, {direct['accuracy_ci'][1]:.4f}]")
        print(f"Firmware macro-F1: {direct['macro_f1']:.4f} [{direct['f1_ci'][0]:.4f}, {direct['f1_ci'][1]:.4f}]")
        print(f"Firmware macro-OvR AUROC: {direct['macro_auc']:.4f} [{direct['auc_ci'][0]:.4f}, {direct['auc_ci'][1]:.4f}]")
        print("\nDirect firmware confusion matrix:")
        print("Labels:", direct["labels"])
        print(direct["cm"])
        print(classification_report(test_fw, direct["pred"], labels=direct["labels"], zero_division=0))

if results:
    labels = [r[0] for r in results]
    slot_auc = np.array([r[1]["macro_auc"] for r in results])
    slot_lo = np.array([r[1]["auc_ci"][0] for r in results])
    slot_hi = np.array([r[1]["auc_ci"][1] for r in results])
    direct_auc = np.array([r[4]["macro_auc"] for r in results])
    direct_lo = np.array([r[4]["auc_ci"][0] for r in results])
    direct_hi = np.array([r[4]["auc_ci"][1] for r in results])
    x = np.arange(len(labels))
    width = 0.36
    fig, ax = plt.subplots(figsize=(9, 5))

    ax.bar(
        x - width / 2,
        slot_auc,
        width,
        yerr=np.vstack([
            slot_auc - slot_lo,
            slot_hi - slot_auc
        ]),
        capsize=5,
        label="Slot-mediated attack"
    )

    ax.bar(
        x + width / 2,
        direct_auc,
        width,
        yerr=np.vstack([
            direct_auc - direct_lo,
            direct_hi - direct_auc
        ]),
        capsize=5,
        label="Direct firmware attack"
    )

    ax.axhline(
        0.5,
        linestyle="--",
        linewidth=1,
        label="Chance AUROC"
    )

    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_ylabel("Macro-OvR AUROC")
    ax.set_ylim(0, 1.1)
    ax.set_title("Independent held-out selection inference")
    ax.legend()
    fig.tight_layout()
    fig.savefig("heldout-selection-inference-auroc.png", dpi=300, bbox_inches="tight")

    plt.show()